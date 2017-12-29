//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_BLOCKWISE_H
#define MC_COAP_TEST_COAP_BLOCKWISE_H

#include <stdint.h>
#include "platform.h"
#include "mc/memory-chunk.h"
#include "coap-uint.h"

namespace moducom { namespace coap {

class BlockOption
{
    // CoAP-formatted UInt
    //uint8_t buffer[3];
    pipeline::layer1::MemoryChunk<3> buffer;
    //experimental::UInt data;

public:
    // all these helper functions expect zero padding in network byte order
    // on 'buffer'.  In other words, if we only get 1 byte from message,
    // buffer[0] and buffer[1] will have zeroes, and buffer[2] will have
    // received byte

    // Aka Block Size.  Compute as real block size = 2**(size_exponent + 4)
    inline uint8_t size_exponent() const { return *buffer.data(2) & 0x7; }

    // aka More Flag ("not last block")
    inline bool more() const { return *buffer.data(2) & 0x08; }

    // aka Block Number
    inline uint32_t sequence_number() const
    {
        return layer2::UInt::get_uint24_t(buffer.data()) >> 4;
    }
};

}}

#endif //MC_COAP_TEST_COAP_BLOCKWISE_H
