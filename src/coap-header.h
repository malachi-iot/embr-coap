//
// Created by malachi on 1/3/18.
//

#ifndef MC_COAP_TEST_COAP_HEADER_H
#define MC_COAP_TEST_COAP_HEADER_H

#include <stdint.h>
#include "platform.h"

namespace moducom { namespace coap {

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

#define COAP_RESPONSE_CODE(_class, _detail) ((_class << 5) | _detail)



// https://tools.ietf.org/html/rfc7252#section-3
class Header
{
    // temporary public while building code
public:
    class Code
    {
        friend class Header;

        uint8_t _code;

        // FIX: for internal use only
        Code() {}
    public:

        Code(uint8_t _code) : _code(_code) {}

        enum Classes
        {
            Request = 0,
            Success = 2,
            ClientError = 4,
            ServerError = 5
        };

        // RFC7252 Section 12.1.2
        enum Codes
        {
            Empty =             COAP_RESPONSE_CODE(0, 00),
            Created =           COAP_RESPONSE_CODE(Success, 01),
            Deleted =           COAP_RESPONSE_CODE(Success, 02),
            Valid =             COAP_RESPONSE_CODE(Success, 03),
            Changed =           COAP_RESPONSE_CODE(Success, 04),
            Content =           COAP_RESPONSE_CODE(Success, 05),
            BadRequest =        COAP_RESPONSE_CODE(ClientError, 00),
            Unauthorized =      COAP_RESPONSE_CODE(ClientError, 01),
            BadOption =         COAP_RESPONSE_CODE(ClientError, 02),
            Forbidden =         COAP_RESPONSE_CODE(ClientError, 03),
            NotFound =          COAP_RESPONSE_CODE(ClientError, 04),
            MethodNotAllowed =  COAP_RESPONSE_CODE(ClientError, 05),
            InternalServerError =   COAP_RESPONSE_CODE(ServerError, 00),
            NotImplemented =        COAP_RESPONSE_CODE(ServerError, 01),
            ServiceUnavailable =    COAP_RESPONSE_CODE(ServerError, 03),
            ProxyingNotSupported =  COAP_RESPONSE_CODE(ServerError, 05)
        };


        // topmost 3 bits
        Classes get_class() const { return (Classes)(_code >> 5); }
        // bottommost 5 bits
        uint8_t detail() const { return _code & 0x1F; }

        Codes code() { return (Codes) _code; }
    };

    // RFC 7252 Section 12.1.1
    enum RequestMethodEnum
    {
        Get = 1,
        Post = 2,
        Put = 3,
        Delete = 4
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
    enum TypeEnum
    {
        Confirmable = 0,
        NonConfirmable = 1,
        Acknowledgement = 2,
        Reset = 3
    };

    TypeEnum type() const
    {
        return (TypeEnum) mask_fixed<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>(1);
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
        mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
    }

    Code& code_experimental() const
    {
        void* buffer = (void*)&bytes[1];
        Code* instance = new (buffer) Code;
        return *instance;
    }

    bool is_request() const
    {
        return (code() >> 5) == 0;
    }

    bool is_response() const { return !is_request(); }

    Code::Codes response_code() const
    {
        ASSERT_WARN(true, is_response(), "Invalid response code detected");

        return Code(code()).code();
    }

    void response_code(Code::Codes value)
    {
        code((Code::Codes)value);
    }

    RequestMethodEnum request_method() const
    {
        return (RequestMethodEnum) code();
    }


    void code(uint8_t code)
    {
        bytes[1] = code;
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
        // slightly clumsy but endianness should be OK
        bytes[2] = mid >> 8;
        bytes[3] = mid & 0xFF;
    }

    uint16_t message_id() const
    {
        uint16_t mid = bytes[2];
        mid <<= 8;
        mid |= bytes[3];
        return mid;
    }

    void token_length(uint8_t tkl)
    {
        ASSERT(true, tkl <= 8);

        bytes[0] &= ~COAP_HEADER_FIXED_TKL_MASK;
        bytes[0] |= tkl;
    }


    uint8_t token_length()
    {
        return bytes[0] & COAP_HEADER_FIXED_TKL_MASK;
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
};

}}

#endif //MC_COAP_TEST_COAP_HEADER_H
