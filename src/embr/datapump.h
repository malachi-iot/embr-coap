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

#include "../exp/datapump-observer.h"
#include "../exp/misc.h"

#ifdef FEATURE_MCCOAP_RELIABLE
#include "../exp/retry.h"
#endif

#ifdef FEATURE_CPP_MOVESEMANTIC
#include <utility> // for std::forward
#endif

namespace embr {

// temporary as we decouple / redo retry logic
using namespace moducom::coap;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
#ifndef FEATURE_CPP_MOVESEMANTIC
#error Move semantic necessary for inline datapump
#endif
#ifndef FEATURE_CPP_VARIADIC
#error Variadic/emplacement necessary for inline datapump
#endif
#endif

#pragma once

// bundle these two together, since they are paired all over the darned place
template <class TNetBuf, class TAddr>
struct TransportDescriptor
{
    typedef TNetBuf netbuf_t;
    typedef TAddr addr_t;
};


template <size_t queue_depth = 10>
struct InlineQueuePolicy
{
    template <class TItem>
    struct Queue
    {
        typedef estd::layer1::queue<TItem, queue_depth> queue_type;
    };
};

struct CoapAppDataPolicy
{
    template <class TTransportDescriptor>
    struct AppData
    {
        typedef typename TTransportDescriptor::netbuf_t netbuf_t;
        typedef typename TTransportDescriptor::addr_t addr_t;

        // NOTE: Not yet used, and not bad but working on decoupling DataPump from coap altogether
        // so a different default policy would be good to supply this AppData
#ifdef FEATURE_MCCOAP_RELIABLE
        typename moducom::coap::experimental::Retry<netbuf_t, addr_t>::Metadata m_retry;
#endif
    };
};


// just a convenient default for datapump
struct CoapAndInlineQueuePolicy :
        InlineQueuePolicy<>,
        CoapAppDataPolicy
{
};

// If this continues to be coap-inspecific, it would be reasonable to move this
// datapump code out to mc-mem.  Until that decision is made, keeping this in
// experimental area
template <class TTransportDescriptor, class TPolicy = CoapAndInlineQueuePolicy >
class DataPump
{
public:
    typedef TTransportDescriptor transport_descriptor_t;
    typedef typename transport_descriptor_t::addr_t addr_t;
    typedef typename transport_descriptor_t::netbuf_t netbuf_t;
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    typedef netbuf_t pnetbuf_t;
#else
    typedef netbuf_t* pnetbuf_t;
#endif
    typedef IDataPumpObserver<netbuf_t, addr_t> datapump_observer_t;
    typedef NetBufDecoder<netbuf_t&> decoder_t;
    typedef TPolicy policy_type;

public:
    // TODO: account for https://tools.ietf.org/html/rfc7252#section-4.2
    // though we have retry.h
    // To track that and other non-core-datagram behavior, stuff everything into AppData
    // via (experimentally) TPolicy.  Deriving from it so that it resolves to truly 0
    // bytes if no AppData is desired
    class Item : policy_type::template AppData<transport_descriptor_t>
    {
        pnetbuf_t m_netbuf;
        addr_t m_addr;

    public:
        Item() {}

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        Item(netbuf_t&& netbuf, const addr_t& addr,
             datapump_observer_t* observer = NULLPTR) :
            m_netbuf(std::forward<netbuf_t>(netbuf)),
#else
        Item(netbuf_t& netbuf, const addr_t& addr,
             datapump_observer_t* observer = NULLPTR) :
            m_netbuf(&netbuf),
#endif
            m_addr(addr)
#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
            ,observer(observer)
#endif
        {}

        Item(const Item& copy_from) :
            m_netbuf(copy_from.m_netbuf),
            m_addr(copy_from.m_addr)
#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
            ,observer(copy_from.observer)
#endif
        {

        }

#if defined(FEATURE_CPP_MOVESEMANTIC) && defined(FEATURE_MCCOAP_DATAPUMP_INLINE)
        Item(Item&& move_from) :
            m_netbuf(std::forward<netbuf_t>(move_from.m_netbuf)),
            m_addr(std::forward<addr_t>(move_from.m_addr))
#ifdef FEATURE_MCCOAP_DATAPUMP_OBSERVABLE
            ,observer(move_from.observer)
#endif
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
        datapump_observer_t* observer;

    public:
        void on_message_transmitting()
        {
            if(observer != NULLPTR)
                observer->on_message_transmitting(netbuf(), addr());
        }


        bool on_message_transmitted()
        {
            if(observer != NULLPTR)
                return observer->on_message_transmitted(netbuf(), addr());

            return false;
        }
#endif
    };


private:
    typedef typename policy_type::template Queue<Item>::queue_type queue_type;

    queue_type incoming;
    queue_type outgoing;

public:
    // process data coming in from transport into coap queue
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    const Item& transport_in(
            netbuf_t&& in,
#else
    void transport_in(
            netbuf_t& in,
#endif
            const addr_t& addr);

    // ascertain whether any -> transport outgoing netbufs are present
    bool transport_empty() const
    {
        return outgoing.empty();
    }

    Item& transport_front()
    {
        return outgoing.front();
    }

    void transport_pop()
    {
        outgoing.pop();
    }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    // enqueue complete netbuf for outgoing transport to pick up
    const Item& enqueue_out(netbuf_t&& out, const addr_t& addr_out, datapump_observer_t* observer = NULLPTR)
    {
        return outgoing.emplace(std::move(out), addr_out, observer);
    }
#else
    // enqueue complete netbuf for outgoing transport to pick up
    bool enqueue_out(netbuf_t& out, const addr_t& addr_out, datapump_observer_t* observer = NULLPTR)
    {
        return outgoing.push(Item(out, addr_out, observer));
    }
#endif

    // see if any netbufs were queued from transport in
    bool dequeue_empty() const { return incoming.empty(); }

    Item& dequeue_front() { return incoming.front(); }

    // TODO: deprecated
    // dequeue complete netbuf which was queued from transport in
    netbuf_t* dequeue_in(addr_t* addr_in)
    {
        if(incoming.empty()) return NULLPTR;

        Item& f = incoming.front();
        netbuf_t* netbuf = f.netbuf();
        *addr_in = f.addr();

        return netbuf;
    }

    // call this after servicing input queued from transport in
    void dequeue_pop()
    {
        incoming.pop();
    }

    // NOTE: Deprecated
    // inline-token, since decoder blasts over its own copy
    struct IncomingContext :
            moducom::coap::IncomingContext<addr_t, true>,
            DecoderContext<decoder_t>
    {
        friend class DataPump;

        typedef typename transport_descriptor_t::netbuf_t netbuf_t;
        typedef moducom::coap::IncomingContext<addr_t, true> base_t;

    private:
        DataPump& datapump;

        // decode header and token and begin option decode
        void prepopulate()
        {
            decoder_t& d = this->decoder();
            base_t::header(d.header());
            // This old code was for non-inline token, but because
            // I was erroneously using token2 (inline) then this call
            // to d.token worked
            //d.token(this->_token);
            this->token(d.token()); // overdoing/abusing it, but should get us there for now
            d.begin_option_experimental();
        }

    public:
        IncomingContext(DataPump& datapump, netbuf_t& netbuf) :
            DecoderContext<decoder_t>(netbuf),
            datapump(datapump)
        {}

        IncomingContext(DataPump& datapump, netbuf_t& netbuf, addr_t& addr) :
            DecoderContext<decoder_t>(netbuf),
            datapump(datapump)
        {
            this->addr = addr;
        }

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

}
