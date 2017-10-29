//
// Created by malachi on 10/21/17.
//

#ifndef SRC_MEMORYPOOL_H
#define SRC_MEMORYPOOL_H

#include <stdint.h>
#include <stdlib.h>

#include "platform.h"


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

    virtual handle_opaque_t copy(handle_opaque_t copy_from,
                                 size_t size,
                                 size_t size_copy) { return invalid_handle; };

    virtual void* lock(handle_opaque_t handle) = 0;
    virtual void unlock(handle_opaque_t handle) = 0;
};

#ifdef USE_PRAGMA_PACK
#pragma pack(1)
#endif

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
            //ASSERT(right.page, page size);

            size += right.size;

            right.set_uninitialized();
        }
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




#ifdef USE_PRAGMA_PACK
#pragma pack()
#endif




}}

#include "mc/memory_pool.h"

#endif //SRC_MEMORYPOOL_H
