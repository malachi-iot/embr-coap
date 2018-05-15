#pragma once

#include <estd/string.h>
#include <coap/platform.h>

template <class TDataPumpHelper>
void simple_uri_responder(TDataPumpHelper& dh, typename TDataPumpHelper::datapump_t& datapump)
{
    using namespace moducom::coap;

    typedef typename TDataPumpHelper::netbuf_t netbuf_t;
    typedef typename TDataPumpHelper::addr_t addr_t;
    typedef typename moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    addr_t ipaddr;

    if(!dh.empty(datapump))
    {
        NetBufDecoder<netbuf_t&> decoder(*dh.front(&ipaddr, datapump));
        layer2::Token token;
        bool gotit = false;
        estd::layer1::string<128> uri;

        Header header_in = decoder.process_header_experimental();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.process_token_experimental(&token);

        decoder.begin_option_experimental();

        // NOTE: Needs to be out here like this, since CoAP presents option numbers
        // as deltas
        Option::Numbers number = Option::Zeroed;

        while(decoder.state() == Decoder::Options)
        {
            decoder.option_experimental(&number);

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << "Option: " << number;
#endif

            if(number == Option::UriPath)
            {
                const estd::layer3::basic_string<char, false> s =
                        decoder.option_string_experimental();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
                std::clog << " (" << s << ')';
#endif

                gotit = s == "test";

                if(uri.size() == 0)
                    uri = s;
                else
                {
                    uri += '/';
                    uri += s;
                }
            }

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif

            // FIX: Not accounting for chunking - in that case
            // we would need multiple calls to option_string_experimental()
            // before calling next
            decoder.option_next_experimental();
        }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        // FIX: Need a much more cohesive way of doing this
        delete &decoder.netbuf();

        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        dh.pop(datapump);

        encoder.header(create_response(header_in,
                                       gotit ? Header::Code::Content : Header::Code::NotFound));
        encoder.token(token);
        // optional and experimental.  Really I think we can do away with encoder.complete()
        // because coap messages are indicated complete mainly by reaching the transport packet
        // size - a mechanism which is fully outside the scope of the encoder
        encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        dh.enqueue(std::forward<netbuf_t>(encoder.netbuf()), ipaddr, datapump);
#else
        dh.enqueue(encoder.netbuf(), ipaddr, datapump);
#endif
    }
}