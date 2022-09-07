/**
 * c++11 version enhances legacy version by using embr::bits::material and friends
 * This approach is architected with many optimization possibilities for big and little
 * endian hosts
 */
#pragma once

#include <estd/cstdint.h>

#include <embr/bits/bits.hpp>

#include "code.h"
#include "enum.h"

namespace embr { namespace coap {

namespace internal { namespace cxx_11 {

// https://tools.ietf.org/html/rfc7252#section-3
class Header : public internal::header::EnumBase,
    public embr::bits::layer1::material<embr::bits::big_endian, 4>
{
    typedef embr::bits::layer1::material<embr::bits::big_endian, 4> base_type;

    // temporary public while building code
public:
    class Code : public internal::header::Code
    {
        friend class Header;

        // FIX: for internal use only
        Code() {}

    public:

        Code(uint8_t code) : internal::header::Code(code) {}
    };

    EMBR_BITS_MATERIAL_PROPERTY(type_native, 0, 4, 2);

    EMBR_BITS_MATERIAL_PROPERTY(version, 0, 6, 2);
    EMBR_BITS_MATERIAL_PROPERTY(token_length, 0, 0, 4);
    EMBR_BITS_MATERIAL_PROPERTY(code, 1, 0, 8)
    EMBR_BITS_MATERIAL_PROPERTY(message_id, 2, 0, 16)

    Types type() const { return (Types) type_native().value(); }
    void type(Types v) { type_native(v); }
};

}}

}}