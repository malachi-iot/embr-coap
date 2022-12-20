#pragma once

#include <estd/cstdint.h>

#define COAP_RESPONSE_CODE(_class, _detail) ((_class << 5) | _detail)

namespace embr { namespace coap {

namespace internal { namespace header {

class Code
{
    uint8_t _code;

protected:
    // For internal use only
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

    // RFC7252 Section 12.1.2 and 12.1.1
    enum Codes
#if __cplusplus >= 201103L
        : uint8_t
#endif
    {
        Empty =             COAP_RESPONSE_CODE(0, 00),
        Get =               COAP_RESPONSE_CODE(0, 01),
        Post =              COAP_RESPONSE_CODE(0, 02),
        Put =               COAP_RESPONSE_CODE(0, 03),
        Delete =            COAP_RESPONSE_CODE(0, 04),
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
        NotAcceptable =             COAP_RESPONSE_CODE(ClientError, 06),
        PreConditionFailed =        COAP_RESPONSE_CODE(ClientError, 12),
        RequestEntityTooLarge =     COAP_RESPONSE_CODE(ClientError, 13),
        UnsupportedContentFormat =  COAP_RESPONSE_CODE(ClientError, 15),
        InternalServerError =   COAP_RESPONSE_CODE(ServerError, 00),
        NotImplemented =        COAP_RESPONSE_CODE(ServerError, 01),
        ServiceUnavailable =    COAP_RESPONSE_CODE(ServerError, 03),
        GatewayTimeout =        COAP_RESPONSE_CODE(ServerError, 04),
        ProxyingNotSupported =  COAP_RESPONSE_CODE(ServerError, 05)
    };


    // topmost 3 bits
    Classes get_class() const { return (Classes)(_code >> 5); }
    // bottommost 5 bits
    uint8_t detail() const { return _code & 0x1F; }

    Codes code() const { return (Codes) _code; }

    bool is_request() const { return get_class() == Request; }
};

}}

}}