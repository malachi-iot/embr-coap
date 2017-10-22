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
#else
#define OVERRIDE
#endif

#define PACKED __attribute__ ((packed))

namespace moducom { namespace coap {

class IMemoryPool
{
public:
    enum TierEnum
    {
        Direct = 0,
        Indexed = 1,
        Compact = 2
    };

    virtual int allocate(size_t size) = 0;
    virtual bool free(int handle) = 0;
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

    handle_t& get_descriptor(uint8_t handle)
    {
        return indexedHandle[handle];
    }

    uint8_t get_page(uint8_t handle)
    {
        return get_descriptor(handle).page;
    }

    size_t handle_size() { return sizeof(handle_t); }

    handle_t& get_first_free(size_t total)
    {
        /* prefer regular for loop cuz indexedHandle is an open ended array
        for(handle_t& handle : indexedHandle)
        {

        }*/
    }
};

template <int page_size = 32, uint8_t page_count = 128>
class MemoryPool : public IMemoryPool
{
    uint8_t pages[page_count][page_size];

public:
    virtual int allocate(size_t size) OVERRIDE { return -1; }
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
