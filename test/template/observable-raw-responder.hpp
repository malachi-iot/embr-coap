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

    std::chrono::milliseconds elapsed = total_since_start;

    elapsed -= last;
    long count = elapsed.count();

    if(count > 1000)
    {
        if(subscribed)
        {
            //std::clog << "Sending to " << addr << std::endl;
            std::clog << "Responding";

            last = total_since_start;
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> encoder;
#else
            NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

            last_header.message_id(last_header.message_id()+ 1);

            Header header = create_response(last_header, Header::Code::Content);

            encoder.header(header);

            //estd::layer1::string<32> payload;
            char payload[32];

            ro_chunk_t _token((const uint8_t*)last_token.data(), last_token.length());
            encoder.token(_token);
            encoder.option(Option::Observe, (int)header.message_id());

            //payload = "Observed! ";
            //payload += header.message_id();
            sprintf(payload, "Observed: %d", header.message_id());

            ro_chunk_t payload_chunk((const uint8_t*)payload, strlen(payload));

            encoder.payload_header();
            // FIX: somehow this one is missing
            //encoder.payload(ro_chunk_t((const uint8_t*)payload, strlen(payload)));
            encoder.write((const void*)payload, strlen(payload));
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
                    response_code = Header::Code::Content;
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
    encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), context.address());
#else
    datapump.enqueue_out(encoder.netbuf(), context.address());
#endif
}
