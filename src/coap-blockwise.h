//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_BLOCKWISE_H
#define MC_COAP_TEST_COAP_BLOCKWISE_H

#include <stdint.h>
#include "platform.h"
#include "mc/memory-chunk.h"

namespace moducom { namespace coap {

namespace experimental {

// FIX: technically this would be a layer2 uint then
// also we may want larger than 4 at some point, but that's
// as big as is convenient for now
class OptionUInt : pipeline::layer2::MemoryChunk<4, uint8_t>
{
public:
    // copy-pastes from OptionExperimental
    uint8_t get_uint8_t() const
    {
        ASSERT_ERROR(1, length(), "Incorrect length");
        return *data();
    }

    uint32_t get_uint16_t() const
    {
        ASSERT_ERROR(2, length(), "Incorrect length");

        const uint8_t* value = data();
        uint32_t v = *value;

        v <<= 8;
        v |= value[1];

        return v;
    }

    static uint32_t get_uint24_t(const uint8_t* value)
    {
        uint32_t v = *value;

        v <<= 8;
        v |= value[1];
        v <<= 8;
        v |= value[2];

        return v;
    }

    uint32_t get_uint24_t() const
    {
        ASSERT_ERROR(3, length(), "Incorrect length");

        const uint8_t* value = data();

        return get_uint24_t(value);
    }


    template <class TOutput>
    inline static void set_uint24_t(uint32_t input, TOutput& output)
    {
        for(int i = 3; i-- > 0; i)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }
    }

    static uint32_t get_uint32_t(const uint8_t* value)
    {
        uint32_t v = *value;

        for(int i = 0; i < 4; i++)
        {
            v <<= 8;
            v |= value[i];
        }

        return v;
    }

    // Have yet to see a CoAP UINT option value larger than 32 bits
    uint32_t get_uint32_t() const
    {
        ASSERT_ERROR(true, length() <= 4, "Option length too large");

        const uint8_t* value = data();

        return get_uint32_t(value);
    }

    template <class TOutput>
    inline static uint8_t set_uint32_t(uint32_t input, TOutput& output)
    {
        uint8_t bytes_used;

        if(input == 0)
            return 0;
        else if(input <= 0xFF)
            bytes_used = 1;
        else if(input <= 0xFFFF)
            bytes_used = 2;
        else if(input <= 0XFFFFFF)
            bytes_used = 3;
        else
            bytes_used = 4;

        for(int i = bytes_used; i-- > 0; i)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }

        return bytes_used;
    }
};

}

class BlockOption
{
    // CoAP-formatted UInt
    //uint8_t buffer[3];
    pipeline::layer1::MemoryChunk<3> buffer;
    //experimental::OptionUInt data;

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
        return experimental::OptionUInt::get_uint24_t(buffer.data()) >> 4;
    }
};

}}

#endif //MC_COAP_TEST_COAP_BLOCKWISE_H
