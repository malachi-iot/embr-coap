#pragma once

#include <estd/cstdint.h>

#define COAP_RESPONSE_CODE(_class, _detail) ((_class << 5) | _detail)

namespace embr { namespace coap {

namespace internal { namespace header {

class Code
{
    uint8_t _code;

protected:
    // DEBT: This empty ctor exists to feed code_experimental in legacy header, which itself
    // uses a technique that got me in trouble with the debouncer code.  Phase this
    // out completely

    // For internal use only
    Code() {}

public:

    ESTD_CPP_CONSTEXPR_RET Code(uint8_t _code) : _code(_code) {}

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
    Classes get_class() const { return (Classes)(_code >> 5); }
    // bottommost 5 bits
    uint8_t detail() const { return _code & 0x1F; }

    Codes code() const { return (Codes) _code; }

    bool is(Classes c) const { return get_class() == c; }

    bool is_request() const { return get_class() == Request; }
    bool success() const { return is(Success); }

    operator Codes () const { return code(); }
    //bool operator==(Codes code) const { return code == _code; }
};

}}

}}