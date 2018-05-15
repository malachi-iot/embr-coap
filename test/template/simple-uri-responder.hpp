#pragma once

#include <estd/string.h>
#include <coap/platform.h>
#include <coap/encoder.hpp>
#include <coap/decoder.h>
#include <coap/context.h>
#include <mc/memory-chunk.h>


template <class TNetBuf, bool inline_token>
void encode_header_and_token(moducom::coap::NetBufEncoder<TNetBuf>& encoder,
                             moducom::coap::TokenAndHeaderContext<inline_token>& context,
                             moducom::coap::Header::Code::Codes response_code)
{
    encoder.header(moducom::coap::create_response(context.header(),
                                   response_code));
    encoder.token(context.token());
}


template <class TDataPump>
void simple_uri_responder(TDataPump& datapump, typename TDataPump::IncomingContext& context)
{
    using namespace moducom::coap;

    typedef typename TDataPump::netbuf_t netbuf_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    experimental_option_iterator<netbuf_t&> it(context.decoder());

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

            default: break;
        }

        // FIX: Not accounting for chunking - in that case
        // we would need multiple calls to option_string_experimental()
        // before calling next
        ++it;
    }

    if(uri == "test/") response_code = Header::Code::Valid;

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

