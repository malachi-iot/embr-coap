/**
 * c++03 version enhances legacy version by using embr::bits::word
 * This has one minor downside in that that only operates in the host endianness
 * Some endian swapping may be necessary to move to CoAP's network order (Big Endian)
 * embr::bits::material and friends are c++11 only
 */
#pragma once

#include <estd/cstdint.h>

#include <embr/bits/word.hpp>

#include "code.h"
#include "enum.h"

namespace embr { namespace coap {

namespace internal { namespace cxx_03 {

// https://tools.ietf.org/html/rfc7252#section-3
class Header : public internal::header::EnumBase,
   public embr::bits::internal::word<32>
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

}}