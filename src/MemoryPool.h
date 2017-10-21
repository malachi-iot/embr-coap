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

namespace moducom { namespace coap {

class IMemoryPool
{
public:
    virtual int allocate(size_t size) = 0;
    virtual bool free(int handle) = 0;
};

template <int page_size = 32, uint8_t page_count = 128>
class MemoryPool : public IMemoryPool
{
    enum TierEnum
    {
        Direct = 0,
        Indexed = 1,
        Compact = 2
    };

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
