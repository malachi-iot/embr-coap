#pragma once

#include "../platform.h"

#if FEATURE_MCCOAP_HEADER_LEGACY_COMPILE
#include "header/legacy.h"
#endif
#include "header/c++03.h"
#if __cplusplus >= 201103L
#include "header/c++11.h"
#endif

namespace embr { namespace coap { namespace internal { namespace header {

// All of the following are helpers for all of the 3 above Header variations.  If this were c++20
// we would apply a concept/requires to enforce the signature.  As it stands, we tuck these
// somewhat vague templatized helpers deep into internal namespacing and will alias/typedef them
// out for the particular system Header technique chosen

// NOTE: Most of these are inactive and therefore untested since we deprecated the old mcmem codebase
// FIX: Due to ADL, these get picked up at a global scope

template <class THeader>
bool is_request(const THeader& h)
{
    return (h.code() >> 5) == 0;
}

template <class THeader>
bool is_response(const THeader& h)
{
    return !is_request(h);
}

template <class THeader>
Code::Codes response_code(const THeader& h)
{
    ASSERT_WARN(true, is_response(h), "Invalid response code detected");

    return Code(h.code()).code();
}


template <class THeader>
void response_code(THeader& h, Code::Codes value)
{
    h.code(value);
}

template <class THeader>
RequestMethods request_method(const THeader& h)
{
    return (RequestMethods) h.code();
}




}}}}
