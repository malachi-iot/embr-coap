//
// Created by malachi on 10/21/17.
//

#ifndef SRC_MEMORYPOOL_H
#define SRC_MEMORYPOOL_H

#include <stdint.h>
#include <stdlib.h>


// TODO: find "most" standardized version of this
#ifdef __CPP11__
#define OVERRIDE override
#define CONSTEXPR constexpr
#define NULLPTR nullptr
#else
#define OVERRIDE
#define CONSTEXPR const
#define NULLPTR NULL
#endif

#define PACKED __attribute__ ((packed))

// TODO: Make this generate log warnings or something
#define ASSERT(expected, actual)

namespace moducom { namespace dynamic {

class IMemoryPool
{
protected:
public:
    typedef int handle_opaque_t;

    static CONSTEXPR handle_opaque_t invalid_handle = -1;

    enum TierEnum
    {
        Direct = 0,
        Indexed = 1,
        Compact = 2,
        Indexed2 = 3
        // Compact2 would be like the linked-list version I was working on before, but might have to carry handle info along too
    };

    virtual handle_opaque_t allocate(size_t size) = 0;
    virtual bool free(handle_opaque_t handle) = 0;
    // TODO: consider making this an append instead
    virtual bool expand(handle_opaque_t handle, size_t size) = 0;
    virtual void shrink(handle_opaque_t handle, size_t size) = 0;
};

/// Fits in the size of one page
///
struct PACKED MemoryPoolDescriptor
{
    IMemoryPool::TierEnum tier : 3;
    // Number of handles in this page
    uint8_t size: 4;
    // Next page also has is a memory pool descriptor
    bool followup: 1;

    struct PACKED IndexedHandle
    {
        /// Is the memory on the page allocated or free
        bool allocated : 1;
        /// What page in the pool does this handle point to
        uint8_t page;
        /// how many pages large this handle is
        /// @name test
        uint8_t size : 7;

        void set_uninitialized()
        {
            allocated = false;
            size = 0;
        }

        bool is_initialized()
        {
            return size > 0;
        }

        /// Combine this + another contiguous free handles into this one handle
        void combine(IndexedHandle& right)
        {
            ASSERT(false, allocated);
            ASSERT(false, right.allocated);
            ASSERT(right.page, page size);

            size += right.size;

            right.set_uninitialized();
        }
    };


    struct PACKED Index2Handle
    {
        /// which page this handle points to.  0 is reserved always for system handle
        /// descriptor area, and therefore is repurposed as an "inactive" indicator
        uint8_t page;

        bool is_active() const { return page != 0; }

        struct PACKED PageData
        {
            bool locked : 1;
            bool allocated : 1;

            /// # of utilized pages valid values from 1-255
            uint8_t size;

            /// Initialize an active but unlocked and unallocated
            /// page
            PageData(uint8_t size)
            {
                locked = false;
                allocated = false;
                this->size = size;
            }

            PageData(const PageData& copy_from)
            {
                *this = copy_from;
            }
        };
    };


    struct CompactHandle : IndexedHandle
    {
        uint8_t handle;
    };
};

// page dedicated to just managing handles
struct PACKED MemoryPoolHandlePage
{
    MemoryPoolDescriptor header;
    //MemoryPoolDescriptor::CompactHandle compactHandle[];
};

struct PACKED MemoryPoolIndexedHandlePage : MemoryPoolHandlePage
{
    typedef MemoryPoolDescriptor::IndexedHandle handle_t;
    typedef IMemoryPool::handle_opaque_t handle_opaque_t;

private:
    handle_t indexedHandle[];

public:
    // initialize with total page_count available to free_page
    void initialize(uint8_t free_page, uint8_t page_count);

    const handle_t& get_descriptor(uint8_t handle) const
    {
        return indexedHandle[handle];
    }

    uint8_t get_page(uint8_t handle) const
    {
        return get_descriptor(handle).page;
    }

    static size_t handle_size() { return sizeof(handle_t); }

    size_t get_total_allocated_pages() const
    {
        size_t total = 0;

        for(int i = 0; i < header.size; i++)
        {
            const handle_t& handle = get_descriptor(i);

            if(handle.allocated) total += handle.size;
        }

        return total;
    }

    /// Get first unallocated handle or nullptr if all present handles are allocated
    /// NOTE: this skips uninitialized handles
    handle_t* get_first_unallocated_handle(size_t total, handle_opaque_t* index);

    // Get first unallocated handle or -1 if all present handles are allocated
    handle_opaque_t get_first_unallocated_handle() const
    {
        for(size_t i = 0; i < header.size; i++)
        {
            const handle_t& descriptor = indexedHandle[i];

            if(!descriptor.allocated)
                return i;
        }

        return IMemoryPool::invalid_handle;
    }

    /// Low-level function does NOT initialize handle_t
    handle_t* get_new_handle(size_t total)
    {
        if(header.size >= total) return NULLPTR;

        // we can expand our active handle index count here since there's room
        return &indexedHandle[header.size++];
    }

    void free(handle_opaque_t handle)
    {
        indexedHandle[handle].allocated = false;
    }

    /// Compact two adjacent and contiguous free handles into specified handle
    /// @param handle left side/first of two free handles
    void combine_adjacent(handle_opaque_t handle)
    {
        handle_t& left = indexedHandle[handle];
        handle_t& right = indexedHandle[handle + 1];

        left.combine(right);
    }

    /// acquires a handle for allocation, but doesn't allocate it yet
    /// differs from get_first_unallocated_handle in that it will create a new handle if
    /// necessary
    /*
    handle_t* get_handle_to_allocate(size_t total)
    {
        handle_t* handle = get_first_unallocated_handle(total);

        if(handle == NULLPTR)
        {
            if(header.size < total)
            {
                // we can expand our active handle index count here since there's room
                handle = &indexedHandle[header.size];
                handle->allocated = false; // just to be extra sure
                header.size++;
            }
        }

        return handle;
    }*/
};


struct PACKED MemoryPoolIndexed2HandlePage : MemoryPoolHandlePage
{
    typedef MemoryPoolDescriptor::Index2Handle handle_t;
    typedef IMemoryPool::handle_opaque_t handle_opaque_t;

private:
    handle_t indexedHandle[];

public:
    const handle_t& get_descriptor(uint8_t handle) const
    {
        return indexedHandle[handle];
    }

    /// initialize with total byte count of page_size
    /// @param blank_page Page 1 from memory pool - note this should *already* have its size field initialized
    void initialize(size_t page_size, handle_t::PageData* blank_page)
    {
        header.tier = IMemoryPool::Indexed2;
        header.size = 1;

        page_size -= sizeof(MemoryPoolHandlePage);

        // NOTE: Just a formality, don't need to do a sizeof since it's exactly one byte
        // doing it anyway cuz should optimize out and saves us if we do have to increase
        // size of handle_t
        for(int i = 1; i < page_size / sizeof(handle_t); i++)
        {
            indexedHandle[i].page = 0;
        }

        indexedHandle[0].page = 1;

        blank_page->allocated = false;
        blank_page->locked = false;
    }


    uint8_t get_page(handle_opaque_t handle) const
    {
        return get_descriptor(handle).page;
    }

    size_t get_approximate_header_size() const
    {
        return 2 ^ (size_t) header.size;
    }

    typedef void (*page_data_iterator_fn)(void* context, handle_opaque_t handle, uint8_t page);

    void iterate_page_data(page_data_iterator_fn callback, void* context = NULLPTR) const
    {
        size_t count = get_approximate_header_size();

        for(int i = 0; i < count; i++)
        {
            const handle_t& h = indexedHandle[i];

            if(h.is_active()) callback(context, i, h.page);
        }
    }

#ifdef __CPP11__
#endif


    handle_opaque_t get_first_inactive_handle() const
    {
        // always reflects more elements than are actually
        // present to optimize lookups in large page sizes
        // (remember each handle we can ascertain as active
        // by itself - by it being page == 0 or not)
        const size_t size_approximate = get_approximate_header_size();

        for(int i = 0; i < size_approximate; i++)
        {
            if(!get_descriptor(i).is_active())
                return i;
        }

        return IMemoryPool::invalid_handle;
    }

    handle_t::PageData& get_page_data(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        ASSERT(true, pages != NULLPTR);

        const handle_t& descriptor = get_descriptor(handle);

        ASSERT(true, descriptor.is_active());

        handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                pages + (page_size * descriptor.page));

        return *p;
    }

    // TBD
    // doesn't assign page size just yet
    // see if we can consolidate with above one
    handle_t::PageData& new_page_data(handle_opaque_t new_handle, uint8_t page, uint8_t* pages, size_t page_size)
    {
        handle_t& descriptor = indexedHandle[new_handle];

        descriptor.page = page;

        handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                pages + (page_size * descriptor.page));

        return *p;
    }

    void* lock(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(false, page_data.locked);

        page_data.locked = true;

        return (uint8_t *)(&page_data) + sizeof(handle_t::PageData);
    }

    void unlock(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(true, page_data.locked);

        page_data.locked = false;
    }

    void free(handle_opaque_t handle, uint8_t* pages, size_t page_size)
    {
        handle_t::PageData& page_data = get_page_data(handle, pages, page_size);

        ASSERT(true, page_data.allocated);
        ASSERT(false, page_data.locked);

        page_data.allocated = false;
    }

    /**!
     * Scouring through active and unallocated handles, find one whose minimum size meets our requirement
     * Does a best-fit match
     * @param minimum
     * @param pages
     * @param page_size
     * @param page_data
     * @return
     */
    handle_opaque_t  get_unallocated_handle(size_t minimum, uint8_t* pages, size_t page_size, handle_t::PageData** page_data)
    {
        const size_t size_approximate = get_approximate_header_size();
        handle_t::PageData* candidate = NULLPTR;
        int candidate_index;

        for(int i = 0; i < size_approximate; i++)
        {
            const handle_t& descriptor = get_descriptor(i);

            if(descriptor.is_active())
            {
                handle_t::PageData* p = reinterpret_cast<handle_t::PageData*>(
                        pages + (page_size * descriptor.page));

                if(!p->allocated && p->size >= minimum)
                {
                    if(candidate != NULLPTR)
                    {
                        if(p->size < candidate->size)
                        {
                            candidate = p;
                            candidate_index = i;
                        }
                    }
                    else
                    {
                        candidate = p;
                        candidate_index = i;
                    }
                }
            }
        }

        if(candidate == NULLPTR) return IMemoryPool::invalid_handle;

        *page_data = candidate;

        return candidate_index;
    }

    size_t get_total_unallocated_bytes(const uint8_t* pages, size_t page_size) const
    {
        typedef const handle_t::PageData page_data_t;
        const size_t size_approximate = get_approximate_header_size();
        size_t total = 0;

        for(int i = 0; i < size_approximate; i++)
        {
            const handle_t& descriptor = get_descriptor(i);

            if(descriptor.is_active())
            {
                page_data_t* p = reinterpret_cast<page_data_t*>(
                        pages + (page_size * descriptor.page));

                if(!p->allocated) total += (p->size * page_size) - sizeof(page_data_t);
            }
        }

        return total;
    }

};



template <size_t page_size = 32, uint8_t page_count = 128>
class MemoryPool : public IMemoryPool
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

    static size_t get_size_in_pages(size_t size)
    {
        size_t size_in_pages = (size + page_size - 1) / page_size;

        return size_in_pages;
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
            size_t size_in_pages = get_size_in_pages(size);

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
                handle_opaque_t new_handle = sys_page.get_first_inactive_handle();

                // get location of current unallocated page data, then increment just past end of it
                // this forms the new_page data representing the shrunken remainder unallocated
                // page data
                uint8_t new_page = sys_page.get_page(handle) + page_data->size;

                // set up new page data at location new page within pages buffer
                // then be sure to assign the page_data size to the new shrunken remainder free memory
                page_data_t& new_page_data = sys_page.new_page_data(new_handle, new_page, pages[0], page_size);

                new_page_data.size = page_data->size - size_in_pages;
                new_page_data.allocated = false;
                new_page_data.locked = false;

            }

            page_data->allocated = true;
            page_data->size = size_in_pages;
        }

        return handle;
    }

    virtual handle_opaque_t allocate(size_t size) OVERRIDE
    {
        // TODO: soon, replace with proper polymorphism
        switch(get_sys_page_descriptor().tier)
        {
            case Indexed2:
                return allocate_index2(size);

            default:
                return allocate_index(size);
        }
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
        size_t total_bytes = 0;
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


class MemoryPoolAggregator : public IMemoryPool
{
public:
    virtual int allocate(size_t size) OVERRIDE { return -1; }
    virtual bool free(int handle) OVERRIDE { return false; }
};

}}

#endif //SRC_MEMORYPOOL_H
