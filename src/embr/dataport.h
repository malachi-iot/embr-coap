#pragma once

#include <estd/type_traits.h>
#include "events.h"
#include "datapump.h"

namespace embr {

// DataPort events, specifically
template <class TDatapump>
struct DataPortEvents :
    // Be careful, TDatapump is being used as transport description.  In this case, it's OK
    event::Transport<TDatapump>,
    event::Datapump<typename TDatapump::Item> {};

// NOTE: For now, this actually is just a test event to make sure our event system
// is working as expected
struct dataport_initialized_event {};


// FIX: needs better name
// right now this actually represents a virtual interface specifically to datapump
template <class TTransportDescription>
struct DataPortVirtual
{
    typedef TTransportDescription transport_descriptor_t;
    typedef typename TTransportDescription::netbuf_t netbuf_t;
    typedef typename TTransportDescription::addr_t addr_t;

    // for now, not overpromising since the virtual version if this would be broken
    // (would only service datapump, not dataport)
    //virtual void service() = 0;

    virtual void enqueue_for_send(netbuf_t&& nb, const addr_t& addr) = 0;
    virtual void enqueue_from_receive(netbuf_t&& nb, const addr_t& addr) = 0;
};


// to virtualize the wrapper
// we don't virtual by default because dataport extends its functionality through
// composition (using alternate TDatapump, TTransport, etc) so as to resolve at
// compile time.
// the situations that virtualized calls are needed are fewer, but present.  That's
// what this wrapper class is for
template <class TDataPort>
struct DataPortWrapper :
    DataPortVirtual<typename std::remove_reference<TDataPort>::type::transport_descriptor_t>
{
    TDataPort dataport;

    typedef typename std::remove_reference<TDataPort>::type dataport_t;

    //typedef typename dataport_t::datapump_t transport_description_t;
    typedef typename dataport_t::netbuf_t netbuf_t;
    typedef typename dataport_t::addr_t addr_t;

    DataPortWrapper(dataport_t& dataport) :
        dataport(dataport) {}

    //virtual void service() override { dataport.service(); }

    virtual void enqueue_for_send(netbuf_t&& nb, const addr_t& addr) override
    {
        dataport.enqueue_for_send(std::move(nb), addr);
    }

    virtual void enqueue_from_receive(netbuf_t&& nb, const addr_t& addr) override
    {
        dataport.enqueue_from_receive(std::move(nb), addr);
    }
};


// Datapump API surface needs to be shared all over an app,
// Subject needs to be tightly bound to DataPump for Observer-Subject to work
// Transport interaction need not be shared all over, but at a low technical
// level, due to metadata behavior, at least *does* need potentially to be shared
// ala a hidden/later inherited field
// So we break up DataPort so that hopefully we can use DataPumpSubject and shed
// app-layer dependencies on transport for good
template <class TDatapump, class TTransportDescriptor, class TSubject>
struct DatapumpSubject
{
    typedef typename std::remove_reference<TDatapump>::type datapump_t;
    typedef typename datapump_t::Item item_t;
    typedef typename datapump_t::netbuf_t netbuf_t;
    typedef typename datapump_t::addr_t addr_t;
    // we do, however, need specifics about our netbuf and addr structure
    // since datapump uses those.  Right now datapump 'supplies its own' but
    // eventually want to hang that off transport descriptor
    typedef TTransportDescriptor transport_descriptor_t;
    typedef DataPortEvents<TDatapump> event;

    TDatapump datapump;
    TSubject subject;

    DatapumpSubject(TSubject& subject) :
        subject(subject) {}

    void service();

    // FIX: Still need a graceful solution for wrapped/unwrapped
    template <class TEvent>
    void notify(const TEvent& e, bool wrapped = true)
    {
        if(wrapped)
        {
            // FIX: This is not gonna work for polymorph-izing service,
            // so tend to that when it's time (right now service() call
            // isn't used virtually from anyone)
            DataPortWrapper<DatapumpSubject&> wrapped(*this);

            subject.notify(e, wrapped);
        }
        else
        {
            subject.notify(e, *this);
        }
    }


    // application send out -> datapump -> (eventual transport send)
    void enqueue_for_send(netbuf_t&& nb, const addr_t& addr);
    // transport receive in -> datapump -> (eventual application process)
    void enqueue_from_receive(netbuf_t&& nb, const addr_t& addr);
};

// DataPump and Transport combined, plus a subject to send out
// notifications via
// wrapped = whether to wrap up 'this' in DataPortWrapper in order to virtualize
// it on the way out.  In the future, perhaps merely extend directly from DataPortWrapper
// when the flag is set
template <class TDatapump, class TTransport, class TSubject, bool wrapped = true>
struct DataPort : DatapumpSubject<
        TDatapump,
        typename TTransport::transport_descriptor_t,
        TSubject>
{
    typedef TTransport transport_t;
    typedef typename TTransport::transport_descriptor_t transport_descriptor_t;

    typedef DatapumpSubject<TDatapump, transport_descriptor_t, TSubject> base_t;

    typedef DataPortEvents<TDatapump> event;
    typedef typename base_t::item_t item_t;
    typedef typename base_t::netbuf_t netbuf_t;
    typedef typename base_t::addr_t addr_t;

    TTransport transport;

    template <class TEvent>
    void notify(const TEvent& e)
    {
        base_t::notify(e, wrapped);
    }

public:
/*
    DataPort() : transport(this)
    {
        notify(dataport_initialized_event {});
    } */

    DataPort(TSubject& subject) :
        base_t(subject),
        transport(this, 5683) // FIX: this transport+CoAP specifitiy is definitely not wanted
    {
        notify(dataport_initialized_event {});
    }

    void service();
};


// TODO: Figure out/remember that trailing return -> decltype typedef
// trick so that I don't have to declare that TDataPort twice.
// Then, remove it as a template parameter and instead put in variadic so that
// we can push initialization to transport constructor
template <class TTransport, class TSubject,
            class TDataPort = DataPort<
                embr::DataPump<
                    typename TTransport::transport_descriptor_t
                    >,
                TTransport,
                TSubject> >
TDataPort make_dataport(TSubject& s)
{
    return TDataPort(s);
}

}
