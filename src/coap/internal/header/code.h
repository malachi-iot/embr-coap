#pragma once

#include <estd/cstdint.h>

#define COAP_RESPONSE_CODE(_class, _detail) ((_class << 5) | _detail)

namespace embr { namespace coap {

namespace internal { namespace header {

class Code
{
    // DEBT: Why is this const?  We want to treat 'Code' more or less
    // like a primitive/intrinsic in which case this would be freely
    // assignable - perhaps I was thinking this indicates more of a
    // gauruntee of assignment?  If so that's (obviously) covered
    // by constructor arrangement
    const uint8_t code_;

public:
    ESTD_CPP_CONSTEXPR_RET Code(uint8_t code) : code_(code) {}

    enum Classes
    {
        Request = 0,
        Success = 2,
        ClientError = 4,
        ServerError = 5,
        Signaling = 7
    };

    // RFC7252 Section 12.1.2 and 12.1.1 as well as
    // RFC8323 Section 11.1 as well as
    // [1] "CoAP Response Codes"
    enum Codes
#if __cplusplus >= 201103L
        : uint8_t
#endif
    {
        Empty =             00,
        Get =               01,
        Post =              02,
        Put =               03,
        Delete =            04,
        Fetch =             05,
        Patch =             06,

        Created =           COAP_RESPONSE_CODE(Success, 01),
        Deleted =           COAP_RESPONSE_CODE(Success, 02),
        Valid =             COAP_RESPONSE_CODE(Success, 03),
        Changed =           COAP_RESPONSE_CODE(Success, 04),
        Content =           COAP_RESPONSE_CODE(Success, 05),
        Continue =          COAP_RESPONSE_CODE(Success, 31),

        BadRequest =                COAP_RESPONSE_CODE(ClientError, 00),
        Unauthorized =              COAP_RESPONSE_CODE(ClientError, 01),
        BadOption =                 COAP_RESPONSE_CODE(ClientError, 02),
        Forbidden =                 COAP_RESPONSE_CODE(ClientError, 03),
        NotFound =                  COAP_RESPONSE_CODE(ClientError, 04),
        MethodNotAllowed =          COAP_RESPONSE_CODE(ClientError, 05),
        NotAcceptable =             COAP_RESPONSE_CODE(ClientError, 06),
        RequestEntityIncomplete =   COAP_RESPONSE_CODE(ClientError,  8),
        Conflict =                  COAP_RESPONSE_CODE(ClientError,  9),
        PreConditionFailed =        COAP_RESPONSE_CODE(ClientError, 12),
        RequestEntityTooLarge =     COAP_RESPONSE_CODE(ClientError, 13),
        UnsupportedContentFormat =  COAP_RESPONSE_CODE(ClientError, 15),
        TooManyRequests =           COAP_RESPONSE_CODE(ClientError, 29),

        InternalServerError =       COAP_RESPONSE_CODE(ServerError, 00),
        NotImplemented =            COAP_RESPONSE_CODE(ServerError, 01),
        ServiceUnavailable =        COAP_RESPONSE_CODE(ServerError, 03),
        GatewayTimeout =            COAP_RESPONSE_CODE(ServerError, 04),
        ProxyingNotSupported =      COAP_RESPONSE_CODE(ServerError, 05),

        CSM =                       COAP_RESPONSE_CODE(Signaling, 01),
        Ping =                      COAP_RESPONSE_CODE(Signaling, 02),
        Pong =                      COAP_RESPONSE_CODE(Signaling, 03),
        Release =                   COAP_RESPONSE_CODE(Signaling, 04),
        Abort =                     COAP_RESPONSE_CODE(Signaling, 05),
    };

    // topmost 3 bits
    ESTD_CPP_CONSTEXPR_RET Classes get_class() const { return (Classes)(code_ >> 5); }
    // bottommost 5 bits
    ESTD_CPP_CONSTEXPR_RET uint8_t detail() const { return code_ & 0x1F; }

    ESTD_CPP_CONSTEXPR_RET bool is(Classes c) const { return get_class() == c; }

    ESTD_CPP_CONSTEXPR_RET bool is_request() const { return get_class() == Request; }
    ESTD_CPP_CONSTEXPR_RET bool success() const { return is(Success); }

    ESTD_CPP_CONSTEXPR_RET operator Codes () const
    { return (Codes) code_; }
};


const char* get_description(Code::Codes c);

ESTD_CPP_CONSTEXPR_RET uint16_t get_http_style(Code c)
{
    return c.detail() + (100 * c.get_class());
}

}}

}}
