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

// https://tools.ietf.org/html/rfc7252#section-3
class Header : public internal::header::EnumBase
    // FIX: Need a layer1 material here, not yet available
    //embr::bits::material<embr::bits::big_endian, 4>
{
    typedef embr::bits::internal::word<32> base_type;

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
};

}}