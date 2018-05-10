#include <exp/datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder-subject.h>

namespace moducom { namespace coap {

template <class TMessageObserver, class TNetBuf>
void process_messageobserver_netbuf(DecoderSubjectBase<TMessageObserver>& ds, TNetBuf& netbuf)
{
    typedef pipeline::MemoryChunk::readonly_t chunk_t;

    netbuf.first();

    // FIX: Need to revise end/next to be more tristate because
    // dispatch() wants to know if this is the last chunk, but
    // next() potentially invalidates the *current* buffer, yet
    // we don't know by our internal netbuf standards whether the
    // *current* chunk is the last one until after calling next()
    //while(!netbuf.end())
    {
        chunk_t chunk(netbuf.processed(), netbuf.length_processed());

        // FIX: above comment is why this is true here
        ds.dispatch(chunk, true);

        netbuf.next();
    }
}

template <class TDataPump, class TDecoderSubject>
void process_messageobserver(TDataPump& datapump, TDecoderSubject& ds)
{
    typedef TDataPump datapump_t;
    typedef typename datapump_t::netbuf_t netbuf_t;
    typedef typename datapump_t::addr_t addr_t;

    addr_t ipaddr;
    netbuf_t* netbuf;

    netbuf = datapump.dequeue_in(&ipaddr);

    // echo back out a raw ACK, with no trickery just raw decoding/encoding
    if(netbuf != NULLPTR)
    {
        //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

        process_messageobserver_netbuf(ds, *netbuf);
        /*
        NetBufDecoder<netbuf_t&> decoder(*netbuf);
        layer2::Token token;

        Header header_in = decoder.process_header_experimental();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.process_token_experimental(&token); */

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        netbuf_t temporary;

        // in this scenario, netbuf gets copied around.  Ideally we'd actually do an emplace
        // but code isn't quite there yet
        netbuf = &temporary;
#else
        // FIX: Need a much more cohesive way of doing this
        delete netbuf;
        netbuf = new netbuf_t;
#endif

        datapump.dequeue_pop();
    }
}

}}
