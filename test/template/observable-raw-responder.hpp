#pragma once

#include <estd/string.h>
#include <coap/platform.h>
#include <coap/encoder.hpp>
#include <coap/decoder.h>
#include <coap/decoder/netbuf.h>
#include <coap/context.h>
#include "coap-uint.h"
#include <mc/memory-chunk.h>
#include "exp/datapump.h"

#include <chrono>

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


template <class TIncomingContext>
void simple_observable_responder(TIncomingContext& context)
{
    using namespace moducom::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    option_iterator<netbuf_t> it(context);

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
                if(it.uint8() == 0)
                {
                    subscribed = true;
                    // NOTE: coap-cli doesn't appear to reflect this in observe mode
                    response_code = Header::Code::Valid;
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
