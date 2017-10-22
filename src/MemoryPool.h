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
};

/// Fits in the size of one page
///
struct PACKED MemoryPoolDescriptor
{
    IMemoryPool::TierEnum tier : 3;
    // Number of handles in this page
    uint8_t size: 4;

    struct IndexedHandle
    {
        bool allocated : 1;
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

    handle_t indexedHandle[];

    const handle_t& get_descriptor(uint8_t handle) const
    {
        return indexedHandle[handle];
    }

    uint8_t get_page(uint8_t handle) const
    {
        return get_descriptor(handle).page;
    }

    static size_t handle_size() { return sizeof(handle_t); }

    // Get first unallocated handle or nullptr if all present handles are allocated
    handle_t* get_first_unallocated_handle(size_t total, int* index)
    {
        for(int i = 0; i < header.size; i++)
        {
            handle_t& descriptor = indexedHandle[i];

            if(!descriptor.allocated)
            {
                *index = i;
                return &descriptor;
            }
        }

        return NULLPTR;
    }

    // Get first unallocated handle or -1 if all present handles are allocated
    IMemoryPool::handle_opaque_t get_first_unallocated_handle() const
    {
        for(size_t i = 0; i < header.size; i++)
        {
            const handle_t& descriptor = indexedHandle[i];

            if(!descriptor.allocated)
                return i;
        }

        return IMemoryPool::invalid_handle;
    }

    handle_t* get_new_handle(size_t total)
    {
        if(header.size >= total) return NULLPTR;

        // we can expand our active handle index count here since there's room
        handle_t* handle = &indexedHandle[header.size];
        handle->allocated = false; // just to be extra sure
        header.size++;

        return handle;
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
        typedef pool_t::handle_t handle_t;

        pool_t* sys_page = (pool_t*)pages[0];

        // TODO: refactor to make size 0 based, to increase usable space
        sys_page->header.size = 1;
        sys_page->header.tier = Indexed;
        // set up initial fully-empty handle
        sys_page->indexedHandle[0].allocated = false;
        // maximum size is page_count - 1 since we're using one page for
        // system operations
        sys_page->indexedHandle[0].size = static_cast<uint8_t>(page_count - 1);
    }

public:
    MemoryPool() { initialize(); }

    void compact() {}

    virtual handle_opaque_t allocate(size_t size) OVERRIDE
    {
        initialize();

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
            size_t size_in_pages = (size + page_size - 1) / page_size;

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
                handle->allocated = true;
                handle->size = size_in_pages;

                // TODO: above handle shall be the one returned

                size_t new_unallocated_size_in_pages = handle->size - size_in_pages;

                handle = sys_page->get_new_handle(total);
                handle->allocated = false;
                handle->size = new_unallocated_size_in_pages;

                return index;
            }
        }

        return invalid_handle;
    }

    virtual bool free(int handle) OVERRIDE { return false; }
};


class MemoryPoolAggregator : public IMemoryPool
{
public:
    virtual int allocate(size_t size) OVERRIDE { return -1; }
    virtual bool free(int handle) OVERRIDE { return false; }
};

}}

#endif //SRC_MEMORYPOOL_H
