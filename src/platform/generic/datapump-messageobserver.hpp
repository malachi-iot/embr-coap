#include <exp/datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder-subject.h>

namespace moducom { namespace coap {

template <class TMessageObserver, class TNetBuf, class TAddr>
void process_messageobserver_netbuf(DecoderSubjectBase<TMessageObserver>& ds, TNetBuf& netbuf, TAddr& addr_incoming)
{
    typedef pipeline::MemoryChunk::readonly_t chunk_t;
    typedef typename TMessageObserver::context_t request_context_t;

    netbuf.first();

    // TODO: Need to assign addr_incoming to the observer, preferably through incoming context
    // Seems like it might be prudent either for DecoderSubjectBase itself to be more tightly coupled
    // to that context *OR* to initiate it/control it explicitly from these helper functions
    // until this happens, we can't actually queue any output messages.  Note also we probably
    // want a pointer to datapump itself in the context
    ds.get_observer().context();

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

    addr_t addr_incoming;
    netbuf_t* netbuf;

    netbuf = datapump.dequeue_in(&addr_incoming);

    if(netbuf != NULLPTR)
    {
        //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

        process_messageobserver_netbuf(ds, *netbuf, addr_incoming);

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
