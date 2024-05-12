#pragma once

#include <estd/span.h>

#include "../../uint.h"

// Experimental primarily due to naming
// from https://tools.ietf.org/html/rfc7959#section-2.1
namespace embr { namespace coap { namespace experimental {

// internal call
constexpr inline uint16_t option_block_decode_szx(uint8_t value)
{
    return value & 0x07;
}

// internal call
inline uint16_t option_block_decode_szx(const estd::span<const uint8_t>& option_value)
{
    return option_block_decode_szx(option_value[option_value.size() - 1]);
}

// internal call, push through least-significant-byte of option_value
// through here
inline uint16_t option_block_decode_szx_to_size(uint8_t value)
{
    value &= 0x07;
    value += 4;
    return 1 << value;
}

inline uint16_t option_block_decode_szx_to_size(const estd::span<const uint8_t>& option_value)
{
    // FIX: const_buffer doesn't yet have a .back(), but it will
    return option_block_decode_szx_to_size(option_value[option_value.size() - 1]);
}

// sequence #
template <class Int = uint32_t>
inline Int option_block_decode_num(const estd::span<const uint8_t>& option_value)
{
    // NUM can be up to 20 bits in size.  If you really feel like using less,
    // you can
    return UInt::get<Int>(option_value) >> 4;
}


constexpr bool option_block_decode_m(const estd::span<const uint8_t>& option_value)
{
    return (option_value[option_value.size() - 1] & 0x08) != 0;
}


}}}