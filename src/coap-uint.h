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
    // fully untested
    template <typename TReturn>
    static TReturn get(const uint8_t* value, size_t len)
    {
        TReturn v = *value;

        for(int i = 1; i < len; i++)
        {
            v <<= 8;
            v |= value[i];
        }

        return v;
    }

    // untested
    template <class TInput, class TOutput>
    inline static void set_padded(TInput input, TOutput& output, size_t output_size)
    {
        uint8_t bytes_used;

        // TODO: Could optimize by detecting type/sizeof(type) and only checking
        // if type is capable of holding values that large
        if(input == 0)
        {
            for(int i = 0; i < output_size; i++)
                output[i] = 0;
            return;
        }
        else if(input <= 0xFF)
            bytes_used = 1;
        else if(input <= 0xFFFF)
            bytes_used = 2;
        else if(input <= 0XFFFFFF)
            bytes_used = 3;
        else
            bytes_used = 4;

        uint8_t bytes_pad = output_size - bytes_used;

        for(int i = 0; i < bytes_pad; i++)
        {
            output[i] = 0;
        }

        // NOTE: Not yet tested
        for(int i = output_size; i-- > bytes_pad; i)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }
    }


    template <class TInput, class TOutput>
    inline static uint8_t set(TInput input, TOutput& output)
    {
        uint8_t bytes_used;

        // TODO: Could optimize by detecting type/sizeof(type) and only checking
        // if type is capable of holding values that large
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


namespace layer1 {

template <size_t buffer_size>
class UInt : public pipeline::layer1::MemoryChunk<buffer_size>
{
    typedef pipeline::layer1::MemoryChunk<buffer_size> base_t;

public:
    inline uint8_t operator[](size_t index) const
    {
        return *(base_t::data(index));
    }

    template <class TMemoryChunk>
    void set(const TMemoryChunk& chunk)
    {
        // input as chunk doesn't work because chunk is raw bytes, not converted int
        // furthermore we WANT raw bytes instead of fiddling with converting back
        // and forth to int
        //moducom::coap::UInt::set_padded(chunk, *this, chunk._length());
    }

    template <typename TReturn>
    TReturn get() const
    {
        // TODO: Needs assert
        return moducom::coap::UInt::get<TReturn>(base_t::data(), base_t::length());
    }
};

}

namespace layer2 {

// FIX: technically this would be a layer2 uint then
// also we may want larger than 4 at some point, but that's
// as big as is convenient for now
template <size_t buffer_size = 4>
class UInt : pipeline::layer2::MemoryChunk<buffer_size, uint8_t>
{
    typedef pipeline::layer2::MemoryChunk<buffer_size, uint8_t> base_t;

public:
    inline uint8_t length() const { return this->_length(); }
    const uint8_t* data() const { return base_t::data(); }

    // copy-pastes from OptionExperimentalDeprecated
    uint8_t get_uint8_t() const
    {
        ASSERT_ERROR(1, length(), "Incorrect length");
        return *data();
    }

    uint16_t get_uint16_t() const
    {
        ASSERT_ERROR(true, length() <= 2, "Length greater than 2, but shouldn't be");

        return moducom::coap::UInt::get<uint16_t>(data(), length());
    }

    uint32_t get_uint24_t() const
    {
        ASSERT_ERROR(true, length() <= 3, "Length greater than 2, but shouldn't be");

        return moducom::coap::UInt::get<uint32_t>(data(), length());
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

    // Have yet to see a CoAP UINT option value larger than 32 bits
    uint32_t get_uint32_t() const
    {
        ASSERT_ERROR(true, length() <= 4, "Option length too large");

        return moducom::coap::UInt::get<uint32_t>(data(), length());
    }


    template <class TInput>
    inline uint8_t set(TInput input)
    {
        uint8_t* data = this->writable_data_experimental();
        uint8_t byte_length = moducom::coap::UInt::set(input, data);
        base_t::_length(byte_length);
    }

    template <typename TReturn>
    TReturn get() const
    {
        // TODO: Needs assert
        return moducom::coap::UInt::get<TReturn>(base_t::data(), base_t::length());
    }

    inline uint8_t operator[](size_t index) const
    {
        return *(base_t::data(index));
    }
};

}

}}
#endif //MC_COAP_TEST_COAP_UINT_H
