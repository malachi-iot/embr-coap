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


template <class TNetBuf>
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
void emit_observe(moducom::coap::NetBufEncoder<TNetBuf>& encoder, int sequence)
#else
void emit_observe(moducom::coap::NetBufEncoder<TNetBuf&>& encoder, int sequence)
#endif
{
    // breaking this out thinking maybe some day we can actually genericize evaluate_emit_observe
    // though would be tricky for early-appearing options

    // zero-copy goodness
    // NOTE: Does not account for chunking, and that would be involved since
    // snprintf doesn't indicate whether things got truncated
    int advance_by = snprintf(
            (char*)encoder.payload(), encoder.size(),
            "Observed: %d", sequence);

    encoder.advance(advance_by);

    /* this way works too!
     *

    estd::layer3::basic_string<char, false> s(0, (char*)encoder.data(), encoder.size());

    s += "Observed: ";
    s += estd::to_string(header.message_id());

    encoder.advance(s.size());
     */
}


// TODO: pass in emit and emit_pre, pre being a NULLPTR default fn ptr for options which need to happen
// *before* Observe (6)
template <class TDataPump, class TObservableCollection>
void emit_observe_helper(TDataPump& datapump,
                         moducom::coap::ObservableRegistrar<TObservableCollection>& observable_registrar)
{

}

// Basically working, however first enqueued message mysteriously disappears
template <class TDataPump, class TObservableCollection>
void evaluate_emit_observe(TDataPump& datapump,
                           moducom::coap::ObservableRegistrar<TObservableCollection>& observable_registrar,
                           std::chrono::milliseconds total_since_start)
{
    using namespace moducom::coap;
    typedef typename TDataPump::netbuf_t netbuf_t;
    typedef typename TDataPump::addr_t addr_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef typename TObservableCollection::iterator iterator;

    static std::chrono::milliseconds last(0);

    std::chrono::milliseconds elapsed = total_since_start - last;

    if(elapsed.count() > 1000)
    {
        last = total_since_start;

        iterator it = observable_registrar.begin();

        while(it != observable_registrar.end())
        {
            //std::clog << "Sending to " << addr << std::endl;
            std::clog << "Event fired" << std::endl;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> encoder;
#else
            NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

            static int mid = 0;

            // we want a NON or CON since observer messages are not a normal response
            // but instead classified as 'notifications'
            Header header(Header::NonConfirmable, Header::Code::Content);

            addr_t addr = (*it).addr;
            const layer2::Token& token = (*it).token;
            // NOTE: it appears coap-cli doesn't like a sequence # of 0
            int sequence = ++(*it).sequence;
            ++it;

            header.message_id(mid++);
            header.token_length(token.length());

            encoder.header(header);
            encoder.token(token);

            encoder.option(Option::Observe, sequence); // using mid also for observe counter since we aren't doing CON it won't matter

            emit_observe(encoder, sequence);

            encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), addr);
#else
            datapump.enqueue_out(encoder.netbuf(), addr);
#endif
        }
    }
}


template <class TIncomingContext, class TObservableCollection>
void simple_observable_responder(TIncomingContext& context,
                                 moducom::coap::ObservableRegistrar<TObservableCollection>& observable_registrar)
{
    using namespace moducom::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    option_iterator<netbuf_t> it(context);

    while(it.valid())
    {
        switch(it)
        {
            case  Option::UriPath:
            {
                uri += it.string();
                uri += '/';
                break;
            }

            case Option::Observe:
                switch(it.uint8())
                {
                    case 0:
                        observable_registrar.do_register(context);

                        // NOTE: coap-cli doesn't appear to reflect this in observe mode
                        response_code = Header::Code::Valid;
                        break;

                    case 1:
                        observable_registrar.do_deregister();
                        response_code = Header::Code::Valid;
                        break;

                    default:
                        response_code = Header::Code::BadOption;
                        break;
                }

                break;

            default: break;
        }

        // FIX: Not accounting for chunking - in that case
        // we would need multiple calls to option_string_experimental()
        // before calling next
        ++it;
    }

    context.deallocate_input();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    NetBufEncoder<netbuf_t> encoder;
#else
    NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

    encoder.header_and_token(context, response_code);
    encoder.payload(uri);

    context.respond(encoder);
}
