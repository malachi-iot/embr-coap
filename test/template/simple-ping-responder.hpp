#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>
#include <embr/datapump.h>
#include <estd/string.h>

using namespace std;
using namespace embr::coap;

template <class TDataPump>
void simple_ping_responder(TDataPump& datapump)
{
    typedef TDataPump datapump_t;
    typedef typename datapump_t::netbuf_t netbuf_t;
    typedef typename datapump_t::addr_t addr_t;
    typedef typename datapump_t::Item item_t;
    typedef NetBufDecoder<netbuf_t&> decoder_t;
    //typedef typename moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    // echo back out a raw ACK, with no trickery just raw decoding/encoding
    //if(!sdh.empty(datapump))
    if(!datapump.dequeue_empty())
    {
        item_t& item = datapump.dequeue_front();

        // hold on to ipaddr for when we enqueue a response
        addr_t ipaddr = item.addr();

        decoder_t decoder(*item.netbuf());

        //clog << " ip=" << ipaddr.sin_addr.s_addr << endl;

#define USE_CONTEXT
#ifndef USE_CONTEXT
        layer2::Token token;

        Header header_in = decoder.header();

        // populate token, if present.  Expects decoder to be at TokenStart phase or
        // at OptionsStart (giving us an empty token)
        decoder.token(&token);
#else
        TokenAndHeaderContext<true, false> ctx;

        ctx.header(decoder.header());
        ctx.token(decoder.token());
#endif

#ifdef FEATURE_MCCOAP_IOSTREAM_NATIVE
        // skip any options
        option_iterator<decoder_t> it(decoder);

        while(it.valid()) ++it;

        if(decoder.state() == Decoder::Payload)
        {
            estd::layer3::const_string payload = decoder.payload();

            std::clog << "Got payload: " << payload << std::endl;
        }
#endif

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        // FIX: Need a much more cohesive way of doing this
        delete &decoder.netbuf();

        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        datapump.dequeue_pop();

        //clog << "mid out=" << header.message_id() << endl;

#ifdef USE_CONTEXT
        encoder.header_and_token(ctx, Header::Code::Content);
#else
        encoder.header(create_response(header_in, Header::Code::Content));
        encoder.token(token);
#endif
        // optional and experimental.  Really I think we can do away with encoder.complete()
        // because coap messages are indicated complete mainly by reaching the transport packet
        // size - a mechanism which is fully outside the scope of the encoder
        encoder.complete();

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
        datapump.enqueue_out(std::move(encoder.netbuf()), ipaddr);
        //sdh.enqueue(std::move(encoder.netbuf()), ipaddr, datapump);
#else
        datapump.enqueue_out(encoder.netbuf(), ipaddr);
        //sdh.enqueue(encoder.netbuf(), ipaddr, datapump);
#endif
    }
}
