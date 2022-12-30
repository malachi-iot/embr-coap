#pragma once

namespace embr { namespace coap {

namespace internal { namespace header {

struct EnumBase
{
    // RFC 7252 Section 12.1.1
    // DEBT: Mild collision with Coap::Codes' version.  We favor that one, so phase out
    // this one
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

// DEBT: Phase this particular flavor out after addressing request_method
// and friends' ADL concerns
typedef EnumBase::RequestMethods RequestMethods;
typedef EnumBase::Types Types;

}}

namespace header {

typedef internal::header::EnumBase::RequestMethods RequestMethods;
typedef internal::header::EnumBase::Types Types;

}

}}