#pragma once

namespace embr { namespace coap {

namespace internal { namespace header {

struct EnumBase
{
    // RFC 7252 Section 12.1.1
    enum RequestMethodEnum
    {
        Get = 1,
        Post = 2,
        Put = 3,
        Delete = 4
    };

    enum TypeEnum
    {
        Confirmable = 0,
        NonConfirmable = 1,
        Acknowledgement = 2,
        Reset = 3
    };

    // NOTE: These typedefs are the new recommended names
    typedef RequestMethodEnum RequestMethods;
    typedef TypeEnum Types;
};

}}

}}