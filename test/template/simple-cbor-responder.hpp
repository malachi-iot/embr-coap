#pragma once

#include <estd/string.h>
#include <coap/encoder.hpp>
#include <coap/decoder/netbuf.h>
#include <mc/memory-chunk.h>
#include <cbor/decoder.h>


template <class TIncomingContext>
void simple_cbor_responder(TIncomingContext& context)
{
    using namespace embr::coap;

    typedef typename TIncomingContext::netbuf_t netbuf_t;
    typedef embr::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    option_iterator<netbuf_t> it(context);
    NetBufDecoder<netbuf_t&>& decoder = context.decoder();

    while(it.valid()) ++it;

    bool has_payload = decoder.has_payload_experimental();

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
    std::cout << "Has payload: " << has_payload << std::endl;
#endif

    if(has_payload)
    {
        using namespace embr;

        cbor::Decoder cbor_decoder;
        ro_chunk_t chunk = decoder.payload_experimental();
        const uint8_t* data = chunk.data();
        int len = chunk.length();

        //while(len > 0)
        while(len > 0 || (len == 0 && cbor_decoder.state() != cbor::Decoder::ItemDone))
        {
            if(cbor_decoder.process_iterate(*data))
            {
                data++;
                len--;
            }

            if(cbor_decoder.state() == cbor::Decoder::LongStart)
            {
                if(cbor_decoder.is_indeterminate_type())
                {
#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
                    std::cout << "Fast forwarding" << std::endl;
#endif
                }
                else
                {
                    int fast_forward_by = cbor_decoder.integer<uint16_t>();

                    if(cbor_decoder.type() == cbor::Decoder::String)
                    {
                        estd::layer3::basic_string<char, false> s(fast_forward_by, (char*)data, fast_forward_by);
#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
                        std::cout << "Got string: '" << s << "' - ";
#endif
                    }

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
                    std::cout << "Fast forwarding: " << fast_forward_by << " bytes" << std::endl;
#endif
                    data += fast_forward_by;
                    len -= fast_forward_by;
                }

                cbor_decoder.fast_forward();
            }
            else if(cbor_decoder.state() == cbor::Decoder::ItemDone)
            {
                int intrinsic_int = cbor_decoder.integer<int16_t>();

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
                std::cout << "Accompanying int: " << intrinsic_int << std::endl;
#endif
            }
            else if(cbor_decoder.state() == embr::cbor::Decoder::HeaderDone)
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

