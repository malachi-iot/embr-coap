#pragma once

#include "../MemoryPool.h"
#include "memory_index.h"
#include "memory_index2.h"

namespace moducom { namespace dynamic {

template <size_t page_size = 32, uint8_t page_count = 128>
class MemoryPool : public IMemory
{
    uint8_t pages[page_count][page_size];

    const MemoryPoolDescriptor& get_sys_page_descriptor() const
    {
        return *((MemoryPoolDescriptor*)pages[0]);
    }

    MemoryPoolIndexedHandlePage& get_sys_page_index() const
    {
        return *((MemoryPoolIndexedHandlePage*)pages[0]);
    }

    MemoryPoolIndexed2HandlePage& get_sys_page_index2() const
    {
        return *((MemoryPoolIndexed2HandlePage*)pages[0]);
    }


    void initialize_index()
    {
        typedef MemoryPoolIndexedHandlePage pool_t;

        pool_t* sys_page = (pool_t*)pages[0];

        sys_page->initialize(1, page_count - 1);
    }

    void initialize_index2()
    {
        typedef MemoryPoolIndexed2HandlePage::handle_t::PageData page_data_t;

        page_data_t* page_data = (page_data_t*)(pages[1]);

        page_data->size = page_count -1;

        get_sys_page_index2().initialize(page_size, page_data);
    }

    static uint8_t get_size_in_pages(size_t size)
    {
        size_t size_in_pages = (size + page_size - 1) / page_size;

        ASSERT(true, size_in_pages < 0xFF);

        return (uint8_t) size_in_pages;
    }

    void* lock_index2(handle_opaque_t handle)
    {
        return get_sys_page_index2().lock(handle, pages[0], page_size);
    }

public:
    MemoryPool(TierEnum tier = Indexed)
    {
        switch(tier)
        {
            case Indexed:
                initialize_index();
                break;

            case Indexed2:
                initialize_index2();
                break;
        }
    }

    void compact() {}

    virtual void* lock(handle_opaque_t handle) OVERRIDE
    {
        switch(get_sys_page_descriptor().tier)
        {
            case Indexed:
                //get_sys_page_index()
                break;

            case Indexed2:
                return lock_index2(handle);
        }

        return NULLPTR;
    }

    virtual void unlock(handle_opaque_t handle) OVERRIDE
    {
        switch(get_sys_page_descriptor().tier)
        {
            case Indexed2:
                get_sys_page_index2().unlock(handle, pages[0], page_size);
        }
    }

    handle_opaque_t allocate_index(size_t size)
    {
        typedef MemoryPoolIndexedHandlePage pool_t;
        typedef pool_t::handle_t handle_t;

        pool_t* sys_page = (pool_t*)pages[0];
        // total = total number of handles that can fit in a page
        CONSTEXPR size_t total = (page_size - sizeof(MemoryPoolDescriptor)) /
                                 sizeof(handle_t);
        handle_opaque_t index;

        handle_t* handle = sys_page->get_first_unallocated_handle(total, &index);

        if(handle != NULLPTR)
        {
            uint8_t size_in_pages = get_size_in_pages(size);

            if(size_in_pages == handle->size)
            {
                // EZ mode, exact size match, set allocated flag and return
                handle->allocated = true;
                // TODO: oops, need to be doing this all with handle #'s my bad
                return index;
            }

            // if requested size is too large, we'll need to attempt a compact and try again
            if(size_in_pages > handle->size)
            {
                // TODO: Don't bother with a compact if we know there aren't discontiguous pages
                compact();

                // TODO: Now try allocation again see if there's more contiguous memory
            }

            // if requested size is smaller than available handle
            if(size_in_pages < handle->size)
            {
                // TODO: Keep scanning looking for a better fit

                // split the free handle in two, with the new handle containing remainder unallocated space
                handle_t* new_handle = sys_page->get_new_handle(total);
                new_handle->allocated = false;
                new_handle->page = handle->page + size_in_pages;
                new_handle->size = handle->size - size_in_pages;

                handle->allocated = true;
                handle->size = size_in_pages;

                return index;
            }
        }

        return invalid_handle;
    }

    handle_opaque_t allocate_index2(size_t size)
    {
        typedef MemoryPoolIndexed2HandlePage pool_t;
        typedef pool_t::handle_t handle_t;
        typedef handle_t::PageData page_data_t;

        pool_t& sys_page = get_sys_page_index2();
        page_data_t* page_data;

        handle_opaque_t handle = sys_page.get_unallocated_handle(size, pages[0], page_size, &page_data);

        // TODO: Do split logic like allocate_index does
        if(handle != invalid_handle)
        {
            size_t size_in_pages = get_size_in_pages(size);

            // Do split logic
            if(page_data->size > size_in_pages)
            {
                // get brand new inactive handle to snap up and use
                handle_opaque_t new_handle = sys_page.get_first_inactive_handle(page_size);

                ASSERT_ERROR(true, new_handle != invalid_handle, "Unable to find inactive handle");

                // get location of current unallocated page data, then increment just past end of it
                // this forms the new_page data representing the shrunken remainder unallocated
                // page data
                uint8_t new_page = sys_page.get_page(handle) + size_in_pages;

                // set up new page data at location new page within pages buffer
                // then be sure to assign the page_data size to the new shrunken remainder free memory
                page_data_t& new_page_data = sys_page.new_page_data(new_handle, new_page, pages[0], page_size);

                new_page_data.size = page_data->size - size_in_pages;
                new_page_data.allocated = false;
                new_page_data.locked = false;

            }

            page_data->allocated = true;
            page_data->size = size_in_pages;

            // TODO: fix this for exponential behavior
            sys_page.header.size++;
        }

        return handle;
    }

    virtual handle_opaque_t allocate(size_t size) OVERRIDE
    {
        const MemoryPoolDescriptor& descriptor = get_sys_page_descriptor();

        // TODO: soon, replace with proper polymorphism
        switch(descriptor.tier)
        {
            case Indexed2:
                return allocate_index2(size);

            default:
                return allocate_index(size);
        }
    }


    // untested
    virtual handle_opaque_t allocate(const void* data, size_t size, size_t size_copy) OVERRIDE
    {
        // FIX: optimize
        handle_opaque_t h = allocate(size);

        void* locked = lock(h);

        if(size_copy == 0) size_copy = size;

        memcpy(locked, data, size_copy);

        unlock(h);

        return h;
    }



    bool free_index(handle_opaque_t handle)
    {
        get_sys_page_index().free(handle);

        return true;
    }


    bool free_index2(handle_opaque_t handle)
    {
        get_sys_page_index2().free(handle, pages[0], page_size);

        return true;
    }



    virtual bool free(handle_opaque_t handle) OVERRIDE
    {
        switch(get_sys_page_descriptor().tier)
        {
            case Indexed2:
                return free_index2(handle);

            default:
                return free_index(handle);
        }
    }

    virtual bool expand(handle_opaque_t handle, size_t size) OVERRIDE
    {
        size_t size_in_pages = get_size_in_pages(size);

        typedef MemoryPoolIndexedHandlePage pool_t;
        typedef pool_t::handle_t handle_t;

        pool_t* sys_page = (pool_t*)pages[0];

        const handle_t& h = sys_page->get_descriptor(handle);

        if(size_in_pages <= h.size)
        {
            // Memory blocks already big enough to accomodate expansion
            return true;
        }
        else
        {
            // TODO: scour for contiguous unallocated memory page
            // if none exists, choose either:
            // a) swap allocated contiguous space elsewhere
            // b) move this allocated memory to a new space which fits the expanded size
            // c) compact operation and attempt a) and b) again
        }

        return false;
    }

    virtual void shrink(handle_opaque_t handle, size_t size) OVERRIDE
    {

    }

    size_t get_free_index() const
    {
        size_t total = page_count - 1;

        typedef MemoryPoolIndexedHandlePage pool_t;
        //typedef pool_t::handle_t handle_t;

        pool_t* sys_page = (pool_t*)pages[0];

        total -= sys_page->get_total_allocated_pages();

        return total * page_size;
    }


    size_t get_free_index2() const
    {
        return get_sys_page_index2().get_total_unallocated_bytes(pages[0], page_size);
    }

    size_t get_free() const
    {
        switch(get_sys_page_descriptor().tier)
        {
            case Indexed:
                return get_free_index();

            case Indexed2:
                return get_free_index2();
        }

        return -1;
    }

    struct get_allocated_handle_count_context
    {
        const MemoryPool& memory_pool;
        // true = filter by allocated
        // false = filter by unallocated
        bool filter_allocated;
        // leave this in here see if VisualDSP allows it
        size_t total_bytes;
        size_t total_handles;
        size_t total_locked;

        get_allocated_handle_count_context(const MemoryPool& memory_pool) : memory_pool(memory_pool)
        {
            total_bytes = 0;
            total_handles = 0;
            total_locked = 0;
        }
    };

    static void get_allocated_handle_count_callback(void* context, handle_opaque_t handle, uint8_t page)
    {
        typedef MemoryPoolIndexed2HandlePage::handle_t::PageData page_data_t;

        get_allocated_handle_count_context* ctx = (get_allocated_handle_count_context*)context;
        const MemoryPool& _this = ctx->memory_pool;

        page_data_t* page_data = (page_data_t*)_this.pages[page];

        if(!(ctx->filter_allocated ^ page_data->allocated))
        {
            ctx->total_bytes += page_data->size * page_size - sizeof(page_data_t);
            ctx->total_handles++;
            ctx->total_locked += page_data->locked;
        }
    }

    /**
     * @brief get_allocated_handle_count
     * @param filter_allocated true (default) return allocated metrics, false return unallocated metrics
     * @return
     */
    size_t get_allocated_handle_count(bool filter_allocated = true)
    {
        get_allocated_handle_count_context context(*this);
        context.filter_allocated = filter_allocated;
        get_sys_page_index2().iterate_page_data(get_allocated_handle_count_callback, &context);
        return context.total_handles;
    }
};


class MemoryPoolAggregator : public IMemory
{
public:
    virtual handle_opaque_t allocate(size_t size) OVERRIDE { return -1; }
    virtual bool free(handle_opaque_t handle) OVERRIDE { return false; }
};

}}
