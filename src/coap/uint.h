#pragma once

#include <estd/cstdint.h>
#include <mc/mem/platform.h>    // needed for ASSERT_WARN
#include <estd/vector.h>
#include <estd/span.h>
#include <estd/array.h>

namespace moducom { namespace coap {

namespace internal {

// lightly tested (16-bit values pushed through)
template <typename TReturn>
inline TReturn uint_get(const uint8_t* value, const size_t len)
{
    // coap cleverly allows 0-length integer buffers, which means value=0
    if(len == 0) return 0;

    TReturn v = *value;

    for(size_t i = 1; i < len; i++)
    {
        v <<= 8;
        v |= value[i];
    }

    return v;
}


template <>
inline uint8_t uint_get<uint8_t>(const uint8_t* value, const size_t len)
{
    // coap cleverly allows 0-length integer buffers, which means value=0
    if(len == 0) return 0;

    ASSERT_WARN(1, len, "len can only be 1 here");

    return *value;
}

}

// where all the static helper methods will live
// This UInt helper class accepts network-order buffers in and decodes them
// to host order
class UInt
{
public:
    typedef estd::span<const uint8_t> const_buffer;

    template <class TReturn>
    static TReturn get(const uint8_t* value, const size_t len = sizeof(TReturn))
    {
        ASSERT_WARN(true, sizeof(TReturn) >= len, "decoding integer size too small");

        return internal::uint_get<TReturn>(value, len);
    }


    template <class TReturn>
    static TReturn get(const const_buffer& b)
    {
        ASSERT_WARN(true, sizeof(TReturn) >= b.size(), "decoding integer size too small");

        return internal::uint_get<TReturn>(b.data(), b.size());
    }


    template <class TReturn, size_t len>
    static TReturn get(const uint8_t (&value) [len])
    {
#ifdef FEATURE_CPP_STATIC_ASSERT
        static_assert(sizeof(TReturn) >= len, "decoding integer size too small");
#else
        ASSERT_WARN(true, sizeof(TReturn) >= len, "decoding integer size too small");
#endif

        return internal::uint_get<TReturn>(value, len);
    }


    // UInt has special behavior where an input value of 0 means
    // 0 bytes are used (more or less NULL = 0 value)
    template <typename TInput>
    inline static uint8_t assess_bytes_used(TInput input, uint8_t bytes_used_headstart = 0)
    {
        uint8_t bytes_used = bytes_used_headstart;

        for(; input != 0; bytes_used++)
            input >>= 8;

        return bytes_used;

        /*
        if(input == 0)
            bytes_used = 0;
        else if(input <= 0xFF)
            bytes_used = 1;
        else if(input <= 0xFFFF)
            bytes_used = 2;
        else if(input <= 0XFFFFFF)
            bytes_used = 3;
        else
            bytes_used = 4;

        return bytes_used; */
    }

    // untested, pads to particular byte size, useful for standard uint16, uint32, etc. sizes
    // like many including CBOR use
    template <class TInput, class TOutput>
    inline static void set_padded(TInput input, TOutput& output, size_t output_size)
    {
        // TODO: Could optimize by detecting type/sizeof(type) and only checking
        // if type is capable of holding values that large
        if(input == 0)
        {
            for(size_t i = 0; i < output_size; i++)
                output[i] = 0;
            return;
        }

        // headstart with 1, since we know we're at least 1 byte big if input != 0
        uint8_t bytes_used = assess_bytes_used(input >> 8, 1);
        /*
        else if(input <= 0xFF)
            bytes_used = 1;
        else if(input <= 0xFFFF)
            bytes_used = 2;
        else if(input <= 0XFFFFFF)
            bytes_used = 3;
        else
            bytes_used = 4; */

        // full output size - actual bytes used yields number of bytes to left-pad with 0
        size_t bytes_pad = output_size - bytes_used;

        for(size_t i = 0; i < bytes_pad; i++)
        {
            output[i] = 0;
        }

        // NOTE: Not yet tested
        for(size_t i = output_size; i-- > bytes_pad;)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }
    }


    template <class TInput, class TOutput>
    inline static void set(TInput input, TOutput& output, uint8_t output_length)
    {
        for(int i = output_length; i-- > 0;)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }
    }


    // taking TOutput& because MemoryChunk gets thrown in here.  Not fully sold
    // on the idea really, the proper simplicity of uint8_t* is hard to beat
    template <class TInput, class TOutput>
    inline static uint8_t set(TInput input, TOutput& output)
    {
        uint8_t bytes_used = assess_bytes_used(input);

        set(input, output, bytes_used);

        return bytes_used;
    }
};


namespace layer1 {

template <size_t buffer_size>
class UInt : public estd::array<uint8_t, buffer_size>
    //public pipeline::layer1::MemoryChunk<buffer_size>
{
    //typedef pipeline::layer1::MemoryChunk<buffer_size> base_t;
    typedef estd::array<uint8_t, buffer_size> base_t;

public:
    /*
    inline uint8_t operator[](size_t index) const
    {
        return *(base_t::data(index));
    } */

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
class UInt : public estd::layer1::vector<uint8_t, buffer_size>
    //pipeline::layer2::MemoryChunk<buffer_size, uint8_t>
{
    typedef estd::layer1::vector<uint8_t, buffer_size> base_t;

public:
    inline uint8_t length() const { return base_t::size(); }
    const uint8_t* data() const { return base_t::clock(); } // FIX: Do explicit lock/unlock

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
        for(int i = 3; i-- > 0;)
        {
            output[i] = input & 0xFF;
            input >>= 8;
        }
    }

    template <class TInput>
    inline void set(TInput input)
    {
        uint8_t* data = base_t::lock();
        uint8_t byte_length = moducom::coap::UInt::set(input, data);
        // FIX: We actually need to do the resize before the set
        base_t::resize(byte_length);
        base_t::unlock();
    }

    template <typename TReturn>
    TReturn get() const
    {
        // TODO: Needs assert
        ASSERT_ERROR(true, length() <= sizeof(TReturn), "Option length too large");
        TReturn ret = moducom::coap::UInt::get<TReturn>(base_t::lock(), base_t::size());
        base_t::unlock();
        return ret;
    }

    inline uint8_t operator[](size_t index) const
    {
        return base_t::at(index);
    }
};

}

}}
