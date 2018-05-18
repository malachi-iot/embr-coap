#pragma once

#include <estd/string.h>
#include <coap/platform.h>
#include <coap/encoder.hpp>
#include <coap/decoder.h>
#include <coap/decoder/netbuf.h>
#include <coap/context.h>
#include <coap/observable.h>
#include "coap-uint.h"
#include <mc/memory-chunk.h>
#include "exp/datapump.h"

#include <chrono>


template <class TNetBuf, class TObservableSession>
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
void emit_observe(moducom::coap::NetBufEncoder<TNetBuf>& encoder, TObservableSession os)
#else
void emit_observe(moducom::coap::NetBufEncoder<TNetBuf&>& encoder, TObservableSession os)
#endif
{
    // zero-copy goodness
    // NOTE: Does not account for chunking, and that would be involved since
    // snprintf doesn't indicate whether things got truncated
    int advance_by = snprintf(
            (char*)encoder.payload(), encoder.size(),
            "Observed: %d", os.sequence);

    encoder.advance(advance_by);

    /* this way works too!
     *

    estd::layer3::basic_string<char, false> s(0, (char*)encoder.data(), encoder.size());

    s += "Observed: ";
    s += estd::to_string(header.message_id());

    encoder.advance(s.size());
     */
}


// NOTE: Perhaps keep this around if std::chrono is available everywhere, otherwise
// back off and just make implementation-specific versions of this.  As it stands this
// is app-specific anyway, 1s repeating emits are not unusual but by far not the only
// notification use case, nor is this the ideal way to do it even if it is the only use case
template <class TDataPump, class TObservableCollection>
void evaluate_emit_observe(TDataPump& datapump,
                           moducom::coap::ObservableRegistrar<TObservableCollection>& observable_registrar,
                           std::chrono::milliseconds total_since_start)
{
    static std::chrono::milliseconds last(0);

    std::chrono::milliseconds elapsed = total_since_start - last;

    if(elapsed.count() > 1000)
    {
        last = total_since_start;

        // iterate over all the registered observers and emit a coap message to them
        // whose contents are determined by emit_observe
        observable_registrar.for_each(datapump, emit_observe);
    }
}


template <class TIncomingContext, class TObservableCollection>
void simple_observable_responder(TIncomingContext& context,
                                 moducom::coap::ObservableRegistrar<TObservableCollection>& observable_registrar)
{
    using namespace moducom::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    option_iterator<netbuf_t> it(context);

    while(it.valid())
    {
        switch(it)
        {
            case  Option::UriPath:
                uri += it.string();
                uri += '/';
                break;

            case Option::Observe:
                response_code = observable_registrar.evaluate_observe_option(
                            context,
                            it.uint8());

                break;

            default: break;
        }

        ++it;
    }

    context.deallocate_input();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    NetBufEncoder<netbuf_t> encoder;
#else
    NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

    // more or less echo back header and token from request
    encoder.header_and_token(context, response_code);
    encoder.payload(uri);

    context.respond(encoder);
}
