#pragma once

#include <estd/string.h>

template <class TDataPumpHelper>
void simple_uri_responder(TDataPumpHelper& dh, typename TDataPumpHelper::datapump_t& datapump)
{
    typedef typename TDataPumpHelper::netbuf_t netbuf_t;
    typedef typename TDataPumpHelper::addr_t addr_t;
    typedef typename moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    addr_t ipaddr;

    if(!dh.empty(datapump))
    {
        NetBufDecoder<netbuf_t&> decoder(*dh.front(&ipaddr, datapump));
        layer2::Token token;

        Header header_in = decoder.process_header_experimental();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.process_token_experimental(&token);

        decoder.process_option_experimental();

        Option::Numbers number;
        uint16_t length;

        decoder.process_option_experimental(&number, &length);

        ro_chunk_t option_value = decoder.get_process_option_experimental();

        // estd non-null terminated strings *not quite* ready for primetime
        //estd::layer3::string s((char*)option_value.data(), option_value.length());

        char buf[128];

        //buf[s.copy(buf, sizeof(buf))] = 0;
        //std::clog << "Got URI: " << s;

        buf[option_value.copy_to(buf, sizeof(buf))] = 0;

        std::clog << "Got URI: " << buf << std::endl;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        // FIX: Need a much more cohesive way of doing this
        delete &decoder.netbuf();

        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        dh.pop(datapump);

        encoder.header(create_response(header_in, Header::Code::Content));
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
