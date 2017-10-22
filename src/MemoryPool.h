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

namespace moducom { namespace coap {

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
        Compact = 2
    };

    virtual int allocate(size_t size) = 0;
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

    struct IndexedHandle
    {
        /// Is the memory on the page allocated or free
        bool allocated : 1;
        /// What page in the pool does this handle point to
        uint8_t page : 7;
        /// how many pages large this handle is
        /// @name test
        uint8_t size;
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

    // Get first unallocated handle or nullptr if all present handles are allocated
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

template <int page_size = 32, uint8_t page_count = 128>
class MemoryPool : public IMemoryPool
{
    uint8_t pages[page_count][page_size];

    void initialize()
    {
        typedef MemoryPoolIndexedHandlePage pool_t;

        pool_t* sys_page = (pool_t*)pages[0];

        sys_page->initialize(1, page_count - 1);
    }

    static size_t get_size_in_pages(size_t size)
    {
        size_t size_in_pages = (size + page_size - 1) / page_size;

        return size_in_pages;
    }

public:
    MemoryPool() { initialize(); }

    void compact() {}

    virtual handle_opaque_t allocate(size_t size) OVERRIDE
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

    virtual bool free(handle_opaque_t handle) OVERRIDE { return false; }

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
    }

    virtual void shrink(handle_opaque_t handle, size_t size) OVERRIDE
    {

    }

    size_t get_free() const
    {
        size_t total = page_count - 1;

        typedef MemoryPoolIndexedHandlePage pool_t;
        //typedef pool_t::handle_t handle_t;

        pool_t* sys_page = (pool_t*)pages[0];

        total -= sys_page->get_total_allocated_pages();

        return total * page_size;
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
