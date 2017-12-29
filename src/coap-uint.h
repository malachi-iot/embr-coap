//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_UINT_H
#define MC_COAP_TEST_COAP_UINT_H

#include <stdint.h>
#include "platform.h"
#include "mc/memory-chunk.h"

namespace moducom { namespace coap {

// where all the static helper methods will live
class UInt
{
public:
};

namespace layer2 {

// FIX: technically this would be a layer2 uint then
// also we may want larger than 4 at some point, but that's
// as big as is convenient for now
class UInt : pipeline::layer2::MemoryChunk<4, uint8_t>
{
public:
    inline uint8_t length() const { return this->_length(); }

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


    // fully untested
    template <typename TReturn>
    static TReturn get(const uint8_t* value, size_t len)
    {
        TReturn v = *value;

        for(int i = 0; i < len; i++)
        {
            v <<= 8;
            v |= value[i];
        }

        return v;
    }


    template <class TInput, class TOutput>
    inline static uint8_t set(TInput input, TOutput& output)
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

    template <class TInput>
    inline uint8_t set(TInput input)
    {
        uint8_t* data = this->writable_data_experimental();
        uint8_t byte_length = set(input, data);
        _length(byte_length);
    }

    inline uint8_t operator[](size_t index) const
    {
        return *(data(index));
    }
};

}

}}
#endif //MC_COAP_TEST_COAP_UINT_H
