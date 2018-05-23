#pragma once

#include <estd/string.h>
#include <coap/encoder.hpp>
#include <coap/decoder/netbuf.h>
#include <mc/memory-chunk.h>
#include <cbor/decoder.h>


template <class TIncomingContext>
void simple_cbor_responder(TIncomingContext& context)
{
    using namespace moducom::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    option_iterator<netbuf_t> it(context);
    NetBufDecoder<netbuf_t&>& decoder = context.decoder();

    while(it.valid()) ++it;

    bool has_payload = decoder.has_payload_experimental();

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
    std::cout << "Has payload: " << has_payload << std::endl;
#endif

    if(has_payload)
    {
        using namespace moducom;

        cbor::Decoder cbor_decoder;
        ro_chunk_t chunk = decoder.payload_experimental();
        const uint8_t* data = chunk.data();
        int len = chunk.length();

        while(len > 0)
        {
            if(cbor_decoder.process_iterate(*data))
            {
                data++;
                len--;
            }

            if(cbor_decoder.state() == cbor::Decoder::LongStart)
            {
                cbor_decoder.fast_forward();
            }
            else if(cbor_decoder.state() == moducom::cbor::Decoder::HeaderDone)
            {
#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
                std::cout << "Has cbor type: " << cbor_decoder.type() << std::endl;
#endif
            }
        }
    }

    context.deallocate_input();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    NetBufEncoder<netbuf_t> encoder;
#else
    NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

    encoder.header_and_token(context, Header::Code::Valid);

    context.respond(encoder);
}

