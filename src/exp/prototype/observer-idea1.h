#pragma once

#include "coap/platform.h"
#include "coap/decoder/observer.h"

// only for observing class/method signatures, no expectation of functional code
namespace experimental { namespace prototype {

namespace idea1 {

class DecoderObserver
{
public:
    template <class TArray>
    DecoderObserver(TArray array) {}

    DecoderObserver() {}
};

template <class TFunc>
class DecoderObserverWithFunc : public DecoderObserver
{
    TFunc func;
public:
    DecoderObserverWithFunc(const TFunc& func) : func(func) {}
};

class URI
{
public:
    //template <class TParam>
    //URI(const char*, const TParam&) {}

    typedef DecoderObserver* (*factory_fn)(const void* context);

private:
    factory_fn factory;
    const void* m_context;

public:

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <int N>
    URI(const char*, URI (&&) [N]) {}
#endif

    template <int N>
    URI(const char*, const URI (&) [N]) {}

    template <class TFunc>
    URI(const char*, const TFunc& f, bool fake_sfinae)
    {

    }

#ifdef FEATURE_CPP_LAMBDA
    template <class TFunc>
    URI(const char*, const TFunc& f)
    {
        m_context = malloc(sizeof(TFunc*));
        m_context = &f;

        factory = [](const void* context)
        {
            const TFunc* ff = ((TFunc*) context);
            DecoderObserver* _do = new DecoderObserverWithFunc<TFunc>(*ff);
            return _do;
        };
    }
#endif
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
