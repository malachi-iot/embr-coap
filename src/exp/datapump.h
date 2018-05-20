#pragma once

#include <estd/forward_list.h>
#include <estd/queue.h>
#include <estd/vector.h>

#include "coap-header.h"
#include "coap-token.h"
#include "coap/context.h"
#include "coap/decoder/netbuf.h"
#include "coap/encoder.h"
#include "coap/observable.h"


// FIX: temporarily putting this here so retry.h doesn't freak out
namespace moducom { namespace coap {

struct IDataPumpObserver
{
    virtual void on_message_transmitting() = 0;
    // LwIP *might* need this, I've heard reports sometimes that after sending a netbuf it goes invalid.  Need to
    // doublecheck because this could strongly affect retry techniques.
    // reusing netbufs is safe, according to:
    // http://lwip.100.n7.nabble.com/netconn-freeing-netbufs-after-netconn-recv-td4145.html
    // For now going to presume we can reuse netbufs for our retry code, but since observer code basically
    // requires a netbuf-copyer, prep that too
    virtual void on_message_transmitted() = 0;
};

}}

#ifdef FEATURE_MCCOAP_RELIABLE
#include "retry.h"
#endif

#ifdef FEATURE_CPP_MOVESEMANTIC
#include <utility> // for std::forward
#endif

namespace moducom { namespace coap {

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
#ifndef FEATURE_CPP_MOVESEMANTIC
#error Move semantic necessary for inline datapump
#endif
#ifndef FEATURE_CPP_VARIADIC
#error Variadic/emplacement necessary for inline datapump
#endif
#endif




// If this continues to be coap-inspecific, it would be reasonable to move this
// datapump code out to mc-mem.  Until that decision is made, keeping this in
// experimental area
template <class TNetBuf, class TAddr, class TAllocator = ::std::allocator<TNetBuf> >
class DataPump
{
public:
    typedef TAddr addr_t;
    typedef TNetBuf netbuf_t;
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    typedef TNetBuf pnetbuf_t;
#else
    typedef TNetBuf* pnetbuf_t;
#endif

public:
    // TODO: account for https://tools.ietf.org/html/rfc7252#section-4.2
    // though we have retry.h, some state wants to be tracked here though eventually
    // I expect we'll do some fancy allocations to have them more directly interact
    class Item
    {
        pnetbuf_t m_netbuf;
        addr_t m_addr;

#ifdef FEATURE_MCCOAP_RELIABLE
        typename experimental::Retry<netbuf_t, addr_t>::Metadata m_retry;
#endif

    public:
        Item() {}

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        Item(TNetBuf&& netbuf, const addr_t& addr) :
            m_netbuf(std::forward<netbuf_t>(netbuf)),
#else
        Item(TNetBuf& netbuf, const addr_t& addr) :
            m_netbuf(&netbuf),
#endif
            m_addr(addr),
            observer(NULLPTR)
        {}

        Item(const Item& copy_from) :
            m_netbuf(copy_from.m_netbuf),
            m_addr(copy_from.m_addr),
            observer(copy_from.observer)
        {

        }

#if defined(FEATURE_CPP_MOVESEMANTIC) && defined(FEATURE_MCCOAP_DATAPUMP_INLINE)
        Item(Item&& move_from) :
            m_netbuf(std::forward<netbuf_t>(move_from.m_netbuf)),
            m_addr(std::forward<addr_t>(move_from.m_addr)),
            observer(move_from.observer)
        {

        }

#endif

        const addr_t& addr() const { return m_addr; }

        netbuf_t* netbuf()
        {
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            return &m_netbuf;
#else
            return m_netbuf;
#endif
        }

#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
    private:
        IDataPumpObserver* observer;

    public:
        void on_message_transmitting()
        {
            if(observer != NULLPTR) observer->on_message_transmitting();
        }


        void on_message_transmitted()
        {
            if(observer != NULLPTR) observer->on_message_transmitted();
        }
#endif
    };


private:
    struct AddrMapper
    {
        addr_t address;
        coap::Header header; // for tkl and mid
        coap::layer1::Token token;
    };

    // does not want to initialize because Item has no default constructor
    // specifically underlying array is an array of Item...
    // TODO: Make underlying array capable of non-initialized data somehow, since
    // a queue is technically that to start with
    estd::layer1::queue<Item, 10> incoming;
    estd::layer1::queue<Item, 10> outgoing;

    //estd::forward_list<> retry;

    // might be better served by 2 different maps or some kind of memory_pool,
    // but we'll roll with just a low-tech vector for now
    estd::layer1::vector<AddrMapper, 10> addr_mapping;

    static bool find_mapper_by_addr(const AddrMapper& mapper)
    {
        return false;
    }

public:
    // process data coming in from transport into coap queue
    void transport_in(
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
            TNetBuf&& in,
#else
            TNetBuf& in,
#endif
            const addr_t& addr);

    // provide a netbuf containing data to be sent out over transport, or NULLPTR
    // if no data is ready
    TNetBuf* transport_front_old(addr_t* addr_out)
    {
        if(outgoing.empty()) return NULLPTR;

        // can't quite do it because it wants AddrMapper (by way of iterator) to have ==
        // but I am not convinced that's the best approach
        //std::find(addr_mapping.begin(), addr_mapping.end(), find_mapper_by_addr);

        Item& f = outgoing.front();

        *addr_out = f.addr();
        TNetBuf* netbuf = f.netbuf();

        return netbuf;
    }

    bool transport_empty()
    {
        return outgoing.empty();
    }

    Item& transport_front()
    {
        return outgoing.front();
    }

    // manually pop Item away, the above transport_out needs to be followed up
    // by this call.  Adding experimental retry hint as a way of transport-specific
    // code indicating an ACK should be expected.
    // Note that this entire datapump code
    // has naturally unfolded as largely coap-inspecific, these ACK/CON interactions
    // may be the first actual coap specificity
    void transport_pop(bool experimental_retry_hint = false)
    {
        // TODO: This is so far the ideal spot to kick off retry queuing
        outgoing.pop();
    }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    // enqueue complete netbuf for outgoing transport to pick up
    bool enqueue_out(TNetBuf&& out, const addr_t& addr_out)
    {
        outgoing.emplace(std::forward<TNetBuf>(out), addr_out);
        return true;
    }
#else
    // enqueue complete netbuf for outgoing transport to pick up
    bool enqueue_out(TNetBuf& out, const addr_t& addr_out)
    {
        return outgoing.push(Item(out, addr_out));
    }
#endif

    // see if any netbufs were queued from transport in
    bool dequeue_empty() { return incoming.empty(); }

    // dequeue complete netbuf which was queued from transport in
    TNetBuf* dequeue_in(addr_t* addr_in)
    {
        if(incoming.empty()) return NULLPTR;

        Item& f = incoming.front();
        TNetBuf* netbuf = f.netbuf();
        *addr_in = f.addr();

        return netbuf;
    }

    // call this after servicing input queued from transport in
    void dequeue_pop()
    {
        incoming.pop();
    }

    // non-inline-token
    struct IncomingContext :
            coap::IncomingContext<addr_t, false>,
            DecoderContext<netbuf_t>
    {
        friend class DataPump;

        typedef TNetBuf netbuf_t;
        typedef NetBufDecoder<netbuf_t&> decoder_t;
        typedef coap::IncomingContext<addr_t, false> base_t;

    private:
        DataPump& datapump;

        void prepopulate()
        {
            decoder_t& d = this->decoder();
            base_t::header(d.header());
            d.process_token_experimental(&this->_token);
            d.begin_option_experimental();
        }

    public:
        IncomingContext(DataPump& datapump, netbuf_t& netbuf) :
            DecoderContext<netbuf_t>(netbuf),
            datapump(datapump)
        {}

        // marks end of input processing
        void deallocate_input()
        {
#ifndef FEATURE_MCCOAP_DATAPUMP_INLINE
            // FIX: Need a much more cohesive way of doing this
            delete &this->decoder().netbuf();
#endif
            datapump.dequeue_pop();
        }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        void respond(NetBufEncoder<netbuf_t>& encoder)
        {
            // lwip netbuf has a kind of 'shrink to fit' behavior which is best applied
            // sooner than later - the complete is perfectly suited to that.
            encoder.complete();
            datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), base_t::address());
        }
#else
        void respond(const NetBufEncoder<netbuf_t&>& encoder)
        {
            encoder.complete();
            datapump.enqueue_out(encoder.netbuf(), base_t::address());
        }
#endif
    };

    //!
    //! \brief service
    //! \param prepopulate_context gathers header, token and initiates option processing
    //!
    void service(void (*f)(IncomingContext&), bool prepopulate_context)
    {
        if(!dequeue_empty())
        {
            // TODO: optimize this so that we can avoid copying addr around
            addr_t addr;

            IncomingContext context(*this, *dequeue_in(&addr));

            context.addr = addr;

            if(prepopulate_context) context.prepopulate();

            f(context);

        }
    }

    //!
    //! \brief service
    //! \param prepopulate_context gathers header, token and initiates option processing
    //!
    template <class TObservableCollection>
    void service(void (*f)(IncomingContext&, ObservableRegistrar<TObservableCollection>&),
                 ObservableRegistrar<TObservableCollection>& observable_registrar,
                 bool prepopulate_context)
    {
        if(!dequeue_empty())
        {
            // TODO: optimize this so that we can avoid copying addr around
            addr_t addr;

            IncomingContext context(*this, *dequeue_in(&addr));

            context.addr = addr;

            if(prepopulate_context) context.prepopulate();

            f(context, observable_registrar);

        }
    }
};

}}
