#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>
#include <exp/datapump.h>
#include <estd/string.h>

using namespace std;
using namespace moducom::coap;

template <class TDataPumpHelper>
void simple_ping_responder(TDataPumpHelper& sdh, typename TDataPumpHelper::datapump_t& datapump)
{
    typedef typename TDataPumpHelper::netbuf_t netbuf_t;
    typedef typename TDataPumpHelper::addr_t addr_t;
    typedef NetBufDecoder<netbuf_t&> decoder_t;
    //typedef typename moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    // echo back out a raw ACK, with no trickery just raw decoding/encoding
    if(!sdh.empty(datapump))
    {
        addr_t ipaddr;

        decoder_t decoder(*sdh.front(&ipaddr, datapump));
        layer2::Token token;

        //clog << " ip=" << ipaddr.sin_addr.s_addr << endl;

        Header header_in = decoder.header();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.token(&token);

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
        // skip any options
        option_iterator<decoder_t> it(decoder, true);

        while(it.valid()) ++it;

        if(decoder.has_payload_experimental())
        {
            estd::layer3::const_string payload = decoder.payload();

            std::clog << "Got payload: " << payload << std::endl;
        }
#endif

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        // FIX: Need a much more cohesive way of doing this
        delete &decoder.netbuf();

        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        sdh.pop(datapump);

        //clog << "mid out=" << header.message_id() << endl;

        encoder.header(create_response(header_in, Header::Code::Content));
        encoder.token(token);
        // optional and experimental.  Really I think we can do away with encoder.complete()
        // because coap messages are indicated complete mainly by reaching the transport packet
        // size - a mechanism which is fully outside the scope of the encoder
        encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        sdh.enqueue(std::forward<netbuf_t>(encoder.netbuf()), ipaddr, datapump);
#else
        sdh.enqueue(encoder.netbuf(), ipaddr, datapump);
#endif
    }
}
