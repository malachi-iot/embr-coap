#pragma once

#include <estd/cstdint.h>

#include "code.h"
#include "enum.h"

namespace embr { namespace coap {

// TODO: Do all these defines with enum if we can
#define COAP_HEADER_FIXED_VER_POS       6
#define COAP_HEADER_FIXED_TYPE_POS      4
#define COAP_HEADER_FIXED_TKL_POS       0

#define COAP_HEADER_FIXED_TYPE_MASK     (3 << COAP_HEADER_FIXED_TYPE_POS)
#define COAP_HEADER_FIXED_TKL_MASK      (15)

#define COAP_HEADER_VER_POS     30
#define COAP_HEADER_TYPE_POS    28
#define COAP_HEADER_TKL_POS     24
#define COAP_HEADER_CODE_POS    16
#define COAP_HEADER_MID_POS     0

#define COAP_HEADER_VER_MASK    (3 << COAP_HEADER_VER_POS)
#define COAP_HEADER_TYPE_MASK   (3 << COAP_HEADER_TYPE_POS)
#define COAP_HEADER_TKL_MASK    (15 << COAP_HEADER_TKL_POS)
#define COAP_HEADER_CODE_MASK   (255 << COAP_HEADER_CODE_POS)
#define COAP_HEADER_MID_MASK    (0xFFFF)

namespace experimental {

inline uint16_t hto_uint16()
{
    return 0;
}


inline void hton(uint16_t input, uint8_t* output)
{
    output[0] = input >> 8;
    output[1] = input & 0xFF;
}


inline uint16_t nto_uint16(const uint8_t* input)
{
    uint16_t value = input[0];
    value <<= 8;
    value |= input[1];
    return value;
}

template <typename T>
inline T ntoh(const uint8_t* bytes);

template <>
inline uint16_t ntoh(const uint8_t* bytes)
{
    return nto_uint16(bytes);
}

}

namespace internal { namespace legacy {

// https://tools.ietf.org/html/rfc7252#section-3
class Header : public internal::header::EnumBase
{
    // temporary public while building code
public:
    class Code : public internal::header::Code
    {
        friend class Header;

        // FIX: for internal use only
        Code() {}
    public:

        Code(internal::header::Code code) : internal::header::Code(code) {}
    };

    union
    {
        uint8_t bytes[4];
        // beware raw exposes endian issues, so only use this for bulk assignment/copy
        uint32_t raw;
    };

protected:

    template <uint8_t pos, uint32_t mask_value>
    inline void mask(uint16_t value)
    {
        raw = (raw & ~mask_value) | (value << pos);
    }

    template <uint8_t pos, uint32_t mask_value>
    inline uint16_t mask() const
    {
        return ((raw & mask_value) >> pos);
    }

    template <uint8_t pos, uint32_t mask_value>
    inline uint8_t mask_fixed(uint8_t byte_pos) const
    {
        return ((bytes[byte_pos] & mask_value) >> pos);
    }

public:

    uint8_t code() const
    {
        return bytes[1];
    }


public:
    TypeEnum type() const
    {
        uint8_t retVal = bytes[0];

        retVal &= COAP_HEADER_FIXED_TYPE_MASK;
        retVal >>= COAP_HEADER_FIXED_TYPE_POS;
        //retVal &= 0b11;   // I like this but it only works in c++11/c++17 and higher

        return (TypeEnum) retVal;
        // NOTE: Pretty sure this one is bugged
        //return (TypeEnum) mask_fixed<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>(1);
    }

protected:
    template <uint8_t pos>
    inline void mask_or(uint8_t byte_pos, uint8_t value)
    {
        bytes[byte_pos] |= value << pos;
    }

public:

    void type(TypeEnum type)
    {
        bytes[0] &= ~COAP_HEADER_FIXED_TYPE_MASK;
        bytes[0] |= ((uint8_t)type) << COAP_HEADER_FIXED_TYPE_POS;
        // FIX: redundant operation.  Looked like mask_or was supposed to take _MASK too but
        // never got around to it
        //mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
    }

    const Code& code_experimental() const
    {
        void* buffer = (void*)&bytes[1];
        Code* instance = new (buffer) Code;
        return *instance;
    }

    void code(uint8_t code)
    {
        bytes[1] = code;
    }


    // Used purely for doing a raw copy.  Much faster than
    // diddling with endianness
    inline void copy_message_id(const Header& from_header)
    {
        bytes[2] = from_header.bytes[2];
        bytes[3] = from_header.bytes[3];
    }


    // https://tools.ietf.org/html/rfc7252#section-3
    /*
     * 16-bit unsigned integer in network byte order.  Used to
      detect message duplication and to match messages of type
      Acknowledgement/Reset to messages of type Confirmable/Non-
      confirmable.  The rules for generating a Message ID and matching
      messages are defined in Section 4.
     */
    void message_id(uint16_t mid)
    {
        experimental::hton(mid, bytes + 2);

        // slightly clumsy but endianness should be OK
        //bytes[2] = mid >> 8;
        //bytes[3] = mid & 0xFF;
    }

    uint16_t message_id() const
    {
        //uint16_t mid = bytes[2];
        //mid <<= 8;
        //mid |= bytes[3];
        //return mid;

        return experimental::ntoh<uint16_t>(bytes + 2);
    }

    void token_length(uint8_t tkl)
    {
        ASSERT(true, tkl <= 8);

        bytes[0] &= ~COAP_HEADER_FIXED_TKL_MASK;
        bytes[0] |= tkl;
    }


    uint8_t token_length() const
    {
        uint8_t tkl = bytes[0] & COAP_HEADER_FIXED_TKL_MASK;

        ASSERT_WARN(true, tkl < 9, "Token length must be < 9");

        return tkl;
    }

    // Should always be 1
    uint8_t version() const
    {
        uint8_t v = bytes[0] >> 6;
        return v;
    }

    //private:
    // FIX: make this private & friended - asking for trouble otherwise
    // with uninitialized data
    Header() {}

public:

    Header(TypeEnum type)
    {
        raw = 0;
        mask_or<COAP_HEADER_FIXED_VER_POS>(0, 1);
        mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
    }


    Header(TypeEnum type, Code::Codes code)
    {
        raw = 0;
        this->code(code);
        mask_or<COAP_HEADER_FIXED_VER_POS>(0, 1);
        mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
    }
};

}}

}}