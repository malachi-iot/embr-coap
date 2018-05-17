#pragma once

#include <estd/string.h>
#include <coap/platform.h>
#include <coap/encoder.hpp>
#include <coap/decoder.h>
#include <coap/context.h>
#include "coap-uint.h"
#include <mc/memory-chunk.h>

#include <chrono>

template <class TNetBuf, bool inline_token>
void encode_header_and_token(moducom::coap::NetBufEncoder<TNetBuf>& encoder,
                             moducom::coap::TokenAndHeaderContext<inline_token>& context,
                             moducom::coap::Header::Code::Codes response_code)
{
    encoder.header(moducom::coap::create_response(context.header(),
                                   response_code));
    encoder.token(context.token());
}

bool subscribed = false;
moducom::coap::Header last_header;
moducom::coap::layer2::Token last_token;

template <class TDataPump>
void evaluate_emit_observe(TDataPump& datapump,
                           typename TDataPump::addr_t addr,
                           std::chrono::milliseconds total_since_start)
{
    using namespace moducom::coap;
    typedef typename TDataPump::netbuf_t netbuf_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    static std::chrono::milliseconds last(0);

    std::chrono::milliseconds elapsed = total_since_start - last;

    if(elapsed.count() > 1000)
    {
        if(subscribed)
        {
            //std::clog << "Sending to " << addr << std::endl;
            std::clog << "Event fired" << std::endl;

            last = total_since_start;
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> encoder;
#else
            NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

            static int mid = 0;

            // we want a NON or CON since observer messages are not a normal response
            Header header(Header::NonConfirmable, Header::Code::Content);

            header.message_id(mid++);
            header.token_length(last_token.length());

            encoder.header(header);
            encoder.token(last_token);
            encoder.option(Option::Observe, mid); // using mid also for observe counter since we aren't doing CON it won't matter

            // zero-copy goodness
            // NOTE: Does not account for chunking, and that would be involved since
            // snprintf doesn't indicate whether things got truncated
            int advance_by = snprintf(
                    (char*)encoder.payload(), encoder.size(),
                    "Observed: %d", header.message_id());

            encoder.advance(advance_by);

            /* this way works too!
             *

            estd::layer3::basic_string<char, false> s(0, (char*)encoder.data(), encoder.size());

            s += "Observed: ";
            s += estd::to_string(header.message_id());

            encoder.advance(s.size());
             */

            encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), addr);
#else
            datapump.enqueue_out(encoder.netbuf(), addr);
#endif
        }
    }
}


template <class TDataPump>
void simple_observable_responder(TDataPump& datapump, typename TDataPump::IncomingContext& context)
{
    using namespace moducom::coap;

    typedef typename TDataPump::netbuf_t netbuf_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    experimental_option_iterator<netbuf_t&> it(context.decoder());

    last_header = context.header();
    last_token = context.token();

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
            {
                uint16_t v = it.template uint<uint16_t>();

                if(v == 0)
                {
                    subscribed = true;
                    // NOTE: coap-cli doesn't appear to reflect this in observe mode
                    response_code = Header::Code::Valid;
                }

                break;
            }

            default: break;
        }

        // FIX: Not accounting for chunking - in that case
        // we would need multiple calls to option_string_experimental()
        // before calling next
        ++it;
    }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    datapump.dequeue_pop();
    NetBufEncoder<netbuf_t> encoder;
#else
    // FIX: Need a much more cohesive way of doing this
    delete &context.decoder().netbuf();
    datapump.dequeue_pop();

    NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

    encode_header_and_token(encoder, context, response_code);

    encoder.payload(uri);

    // optional and experimental.  Really I think we can do away with encoder.complete()
    // because coap messages are indicated complete mainly by reaching the transport packet
    // size - a mechanism which is fully outside the scope of the encoder
    // That said, lwip netbuf does have a kind of 'shrink to fit' behavior which is best applied
    // sooner than later - the complete is perfectly suited to that.
    // Technically the enqueue_out could call the complete() function
    encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), context.address());
#else
    datapump.enqueue_out(encoder.netbuf(), context.address());
#endif
}
