#pragma once

#include <estd/span.h>

#if FEATURE_STD_ALGORITHM
#include <cmath>
#endif

#include "uint.h"

#include "internal/rfc7959/decode.h"
#include "internal/rfc7959/state-machine.h"

// Experimental primarily due to naming
// from https://tools.ietf.org/html/rfc7959#section-2.1
namespace embr { namespace coap { namespace experimental {

// NOTE: remember szx needs some minor math to represent size
template <typename Int>
uint8_t option_block_encode(uint8_t* option_value, Int num, bool m, uint8_t szx)
{
    // DEBT: Do this up with an estd simulated static_assert like in
    // https://gist.github.com/Bueddl/2e4dea884982c22718ceafbebbd75c5f
    //static_assert(!estd::numeric_limits<TUInt>::is_signed);

    // use outgoing option_value as our own variable a bit
    *option_value = 0;

    // shifting over, as num sits 4 bits to the msb of m+szx
    uint8_t size = UInt::set(num << 4, option_value);

    // remember it's implicitly already in bit position 1, so to get it to 4 we shift by 3
    option_value[size - 1] |= m << 3;
    option_value[size - 1] |= szx;

    // possibilities:
    // num size > 0, so always return size in that case
    // num size == 0, when m == false and szx == 0 then uint folds to size 0
    // num size == 0  when m == true or szx > 0 then uint will be size 1
    if(size > 0) return size;

    return *option_value == 0 ? 0 : 1;
}


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



}}}