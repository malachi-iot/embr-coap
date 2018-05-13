#pragma once

#include "coap/platform.h"

// only for observing class/method signatures, no expectation of functional code
namespace experimental { namespace prototype {

namespace idea1 {

class DecoderObserver;

class URI
{
public:
    //template <class TParam>
    //URI(const char*, const TParam&) {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <int N>
    URI(const char*, URI (&&) [N]) {}
#endif

    template <int N>
    URI(const char*, const URI (&) [N]) {}

    template <class TFunc>
    URI(const char*, const TFunc&) {}
};


class DecoderObserver
{
public:
    template <class TArray>
    DecoderObserver(TArray array) {}

    DecoderObserver() {}
};

// dummy decoder subject
template <class TInputContext, class TMessageObserver, int N>
void decoder_observer(TMessageObserver mo, TInputContext& ctx, uint8_t (&a) [N])
{

}


}

namespace idea2 {

#ifdef FEATURE_CPP_VARIADIC
#endif

}

}}
