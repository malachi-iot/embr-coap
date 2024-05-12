#pragma once

#include "../../option.h"

#include "decode.h"
#include "encode.h"

// Experimental primarily due to naming
// from https://tools.ietf.org/html/rfc7959#section-2.1
namespace embr { namespace coap { namespace experimental {



template <class Int>
struct OptionBlock
{
    static_assert(estd::numeric_limits<Int>::is_signed == false,
        "Only unsigned integers supported 'num'");

    Int sequence_num;
    unsigned size : 11;
    bool more : 1;

// DEBT: Really needs a FEATURE_STD_MATH, but this will do
#if FEATURE_STD_ALGORITHM
    uint8_t encode(uint8_t* option_value)
    {
        unsigned v = std::log2(size);
        uint8_t szx = v - 4;
        return option_block_encode(option_value, sequence_num, more, szx);
    }
#endif

    void decode(const estd::span<const uint8_t>& option_value)
    {
        sequence_num = option_block_decode_num(option_value);
        more = option_block_decode_m(option_value);
        size = option_block_decode_szx_to_size(option_value);
    }
};

// DEBT: Combine this with OptionBlock
template <typename Unsigned>
struct OptionBlock2
{
    using value_type = Unsigned;

    Option::ContentFormats content_format_;
    Unsigned total_size_;
    Unsigned current_block_ : (sizeof(Unsigned) * 8) - 4;
    bool more_ : 1;
    Unsigned szx_ : 3;

    Unsigned block_size() const { return 1 << (szx_ + 4); }
    void block_size(Unsigned size)
    {
        unsigned v = std::log2(size);
        szx_ = v - 4;
    }

    // In context of REQUEST_RECEIVING, help determine if final payload size is correct
    Unsigned remaining() const
    {
        return total_size_ - (current_block_ * block_size());
    }

    uint8_t encode(uint8_t* output) const
    {
        return option_block_encode(output, current_block_, more_, szx_);
    }
};



}}}
