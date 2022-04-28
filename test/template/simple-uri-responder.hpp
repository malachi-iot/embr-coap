#pragma once

#include <estd/string.h>
#include <coap/encoder.hpp>
#include <coap/decoder/netbuf.h>
#include <mc/memory-chunk.h>


template <class TIncomingContext>
void simple_uri_responder(TIncomingContext& context)
{
    using namespace embr::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;
    typedef typename TIncomingContext::decoder_t decoder_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    option_iterator<decoder_t> it(context);

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

