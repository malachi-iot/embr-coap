#pragma once

#include <estd/span.h>

#include "uint.h"

// Experimental primarily due to naming
// from https://tools.ietf.org/html/rfc7959#section-2.1
namespace embr { namespace coap { namespace experimental {

// internal call, push through least-significant-byte of option_value
// through here
uint16_t option_block_decode_szx(uint8_t value)
{
    value &= 0x07;
    value += 4;
    return 1 << value;
}

uint16_t option_block_decode_szx(const estd::span<const uint8_t>& option_value)
{
    // FIX: const_buffer doesn't yet have a .back(), but it will
    return option_block_decode_szx(option_value[option_value.size() - 1]);
}

// sequence #
template <class TInt = uint32_t>
TInt option_block_decode_num(const estd::span<const uint8_t>& option_value)
{
    // NUM can be up to 20 bits in size.  If you really feel like using less,
    // you can
    return UInt::get<TInt>(option_value) >> 4;
}


bool option_block_decode_m(const estd::span<const uint8_t>& option_value)
{
    return (option_value[option_value.size() - 1] & 0x08) != 0;
}


// NOTE: remember szx needs some minor math to represent size
template <typename TUInt>
uint8_t option_block_encode(uint8_t* option_value, TUInt num, bool m, uint8_t szx)
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

}}}