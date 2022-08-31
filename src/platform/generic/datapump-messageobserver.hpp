#include <embr/datapump.h>
#include <embr/events.h>

//#include <obsolete/coap/encoder.h>
#include <coap/decoder/netbuf.h>
//#include <obsolete/coap/decoder/subject.h>

namespace embr { namespace coap {

/*
 * Commented out as part of mc-mem removal
template <class TMessageObserver, class TNetBuf>
void process_messageobserver_helper(DecoderSubjectBase<TMessageObserver>& ds, TNetBuf& netbuf,
    typename TMessageObserver::context_t& context)
{
    //typedef pipeline::MemoryChunk::readonly_t chunk_t;
    typedef estd::const_buffer chunk_t;
    //typedef typename TMessageObserver::context_t request_context_t;

    netbuf.first();

    // TODO: Need to assign addr_incoming to the observer, preferably through incoming context
    // Seems like it might be prudent either for DecoderSubjectBase itself to be more tightly coupled
    // to that context *OR* to initiate it/control it explicitly from these helper functions
    // until this happens, we can't actually queue any output messages.  Note also we probably
    // want a pointer to datapump itself in the context
    // NOTE: Don't do this just yet, even though it's basically correct, our consuming unit test
    // is counting on this context remaining in scope
    //ds.get_observer().context(request_context);

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

template <class TMessageObserver, class TNetBuf, class TAddr>
void process_messageobserver_netbuf(DecoderSubjectBase<TMessageObserver>& ds, TNetBuf& netbuf, TAddr& addr_incoming)
{
    typedef typename TMessageObserver::context_t incoming_context_t;
    typedef typename TMessageObserver::context_traits_t request_context_traits;

    incoming_context_t request_context;

    request_context_traits::set_address(request_context, addr_incoming);

    process_messageobserver_helper(ds, netbuf, request_context);
}
*/

template <class TDataPump, class TDecoderSubject>
void process_messageobserver(TDataPump& datapump, TDecoderSubject& ds)
{
    typedef TDataPump datapump_t;
    typedef typename datapump_t::netbuf_type netbuf_t;
    typedef typename datapump_t::endpoint_type addr_t;

    addr_t addr_incoming;
    netbuf_t* netbuf;

    netbuf = datapump.dequeue_in(&addr_incoming);

    if(netbuf != NULLPTR)
    {
        //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

        process_messageobserver_netbuf(ds, *netbuf, addr_incoming);

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
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


// 2nd-generation subject/observer code designed for use with estd::experimental::subject code
// merely evaluates presence of incoming-from-transport data, and if present, fires off
// notification to subject
// FIX: Obviously needs better name
template <class TDataPump, class TSubject>
void process_messageobserver2(TDataPump& datapump, TSubject& subject)
{
    typedef TDataPump datapump_t;
    typedef typename datapump_t::Item item_t;
    //typedef typename datapump_t::addr_t addr_t;

    if(!datapump.dequeue_empty())
    {
        item_t& item = datapump.dequeue_front();

        subject.notify(embr::event::ReceiveDequeuing<item_t>(item));
        subject.notify(embr::event::ReceiveDequeued<item_t>(item));

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
#else
        typedef typename datapump_t::netbuf_t netbuf_t;
        // FIX: Need a much more cohesive way of doing this
        netbuf_t* netbuf = item.netbuf();
        delete netbuf;
#endif

        datapump.dequeue_pop();
    }
}

}}
