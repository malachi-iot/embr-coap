#pragma once

#pragma once

#include "../platform.h"
#include "../MemoryPool.h"

namespace moducom { namespace dynamic {

#ifdef USE_PRAGMA_PACK
#pragma pack(1)
#endif

struct PACKED MemoryPoolIndexedHandlePage : MemoryPoolHandlePage
{
    typedef MemoryPoolDescriptor::IndexedHandle handle_t;
    typedef IMemory::handle_opaque_t handle_opaque_t;

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

        return IMemory::invalid_handle;
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

#ifdef USE_PRAGMA_PACK
#pragma pack()
#endif


}}
