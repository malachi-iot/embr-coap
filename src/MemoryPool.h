//
// Created by malachi on 10/21/17.
//

#ifndef SRC_MEMORYPOOL_H
#define SRC_MEMORYPOOL_H

#include <stdint.h>

namespace moducom { namespace coap {

template <int page_size = 32, uint8_t page_count = 128>
class MemoryPool
{
    enum TierEnum
    {
        Direct = 0,
        Indexed = 1,
        Compact = 2
    };

    uint8_t pages[page_count][page_size];

public:

};

}}

#endif //SRC_MEMORYPOOL_H
