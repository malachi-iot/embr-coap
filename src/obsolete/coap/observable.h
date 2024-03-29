#pragma once

//#include "decoder/subject.h"
//#include "encoder.h"
#include "coap/platform.h"
#include "context.h"
#include <estd/vector.h>

namespace embr { namespace  coap {

// Not seeing anything specific in RFC7641 mandating or denying the use of CON or NON
// messaging for observables, thereby implying we don't need to track what flavor
// we are in this context either
// TODO: Use specialization to make an ObservableSession with a 'void' key
template <class TAddr, class TResourceKey = int>
struct ObservableSession
{
    typedef TAddr addr_t;
    typedef TResourceKey key_t;

    TResourceKey key; // indicates which resource in particular this observable session subscribed to

    // increases with each observe notification.  Technically only needs to be 24 bits
    // so if we do need flags, we can make this into a bit field.  Also, as suggested
    // in RFC7641, we might do without a sequence# altogether and instead use a mangled
    // version of system timestamp.  In that case, holding on to sequence# here might
    // be a waste.  What does not seem to be specified at all is how to
    // handle rollover/overflow conditions.
    // FIX: Only works properly when fully regenerating encoded netbuf per notification
    // otherwise, everything is going to end up copying the initial sequence
    uint32_t sequence;

    // we are the subject, so addr is address of observer - the one
    // who subscribed to us
    TAddr addr;

    // token is one presented during initial subscription
    layer2::Token token;
};


// A registrar represents ONE subject's list of listeners.  By subject
// we generally mean one subscribable URI path
template <class TCollection,
          class TIncomingContext = AddressContext<
              typename TCollection::value_type::addr_t
              >,
          class TRequestContextTraits = incoming_context_traits<TIncomingContext> >
class ObservableRegistrar
{
    typedef estd::const_buffer ro_chunk_t;
    //typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    // Enumeration shall be of type ObservableSession, or something similar to it
    TCollection registrations;

    typedef typename TCollection::value_type observable_session_t;
    typedef typename TCollection::iterator iterator;

public:
    typedef TIncomingContext request_context_t;
    typedef typename request_context_t::addr_t addr_t;
    typedef observable_session_t key_t;


    template <bool inline_token>
    void do_register(const IncomingContext<addr_t, inline_token>& context)
    {
        observable_session_t os;

        os.sequence = 0;
        os.token = context.token();
        os.addr = context.address();

        registrations.push_back(os);
    }



    template <bool inline_token>
    void do_deregister(const IncomingContext<addr_t, inline_token>& context)
    {

    }

    iterator begin()
    {
        return registrations.begin();
    }

    iterator end()
    {
        return registrations.end();
    }


    template <bool inline_token>
    Header::Code::Codes evaluate_observe_option(const IncomingContext<addr_t, inline_token>& context, uint8_t code)
    {
        switch(code)
        {
            case 0:
                do_register(context);

                // NOTE: coap-cli doesn't appear to reflect this in observe mode
                return Header::Code::Valid;

            case 1:
                do_deregister(context);
                return Header::Code::Valid;

            default:
                return Header::Code::BadOption;
        }
    }


    // NetBuf code is fully displaced by estd streambufs
#ifdef OBSOLETE
    // Iterate over each registration in this observer, invoking the emit_observe
    // function per each.  Note that observable_session also shall contain a resource
    // key which can be filtered within the fn, though ultimately we'd like to filter
    // before it gets there
    template <class TDataPump>
    void for_each(
        TDataPump& datapump,
        void (*emit_observe_fn)(
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
                            NetBufEncoder<typename TDataPump::netbuf_t>& encoder,
#else
                            NetBufEncoder<typename TDataPump::netbuf_t&>& encoder,
#endif
                            const observable_session_t& seq),
            bool autosend_observe_option = true);
#endif


    // When evaluating a registration or deregistration, utilize this context
    class Context
    {
    public:
        // true = is registering
        // false = is deregistering
        const bool is_registering;

        // will be carrying address info
        const request_context_t& incomingContext;

        Context(const request_context_t& incomingContext, bool is_registering) :
            incomingContext(incomingContext),
            is_registering(is_registering)
        {
        }
    };

    // when ObservableOptionObserverBase finds a registration or deregistration,
    // call this
    void on_uri_path(Context& context, const ro_chunk_t& chunk, bool last_chunk) {}

    // when ObservableOptionObserverBase completes a registration or deregistration gather
    // call this
    void on_complete(Context& context)
    {
        observable_session_t test;

        test.token = context.incomingContext.token();
        //test.addr = context.incomingContext.address();

        // ObservableSession equality shall rest on the token value
        if(context.is_registering)
        {
            registrations.push_front(test);
        }
        else
        {
            // TODO: need session equality operator in place
            //registrations.remove(test);
        }
    }
};

namespace layer1 {

template<class TAddr, size_t N,
        class TResourceKey = int,
        class TObservableSession = ObservableSession<TAddr, TResourceKey> >
class ObservableRegistrar :
        public embr::coap::ObservableRegistrar<
                estd::layer1::vector<TObservableSession, N> >
{
};

}

/*
 * Commented out as part of mc-mem removal
template <class TRequestContext = ObserverContext>
class ObservableOptionObserverBase : public experimental::MessageObserverBase<TRequestContext>
{
    typedef experimental::MessageObserverBase<TRequestContext> base_t;
    typedef typename base_t::context_t request_context_t;
    typedef typename base_t::context_traits_t request_context_traits;
    typedef typename base_t::context_t::addr_t addr_t;
    typedef ObservableSession<addr_t> observable_session_t;

    typedef typename base_t::option_number_t option_number_t;
    typedef typename base_t::ro_chunk_t ro_chunk_t;

    // FIX: Once LinkedListPool is operational use that for our allocator
    // or at least have it as a default template parameter for ObservableOptionObserverBase itself
    typedef estd::internal::forward_list<
            observable_session_t,
                estd::experimental::ValueNode<
                    observable_session_t,
                    estd::experimental::forward_node_base
                >
            >
            enumeration_t;

    typedef ObservableRegistrar<enumeration_t> registrar_t;

    // specifically leave this uninitialized, on_option must be the one
    // to initialize it
    bool is_registering;

    // FIX: mainly for experimentation at this time, though I expect it
    // will look similar when it settles down
    registrar_t registrar;

public:
    ObservableOptionObserverBase(request_context_t& context) :
        base_t(context)
    {}

    void on_option(option_number_t number, uint16_t len) {}

    void on_option(option_number_t number, const ro_chunk_t& chunk, bool last_chunk);

    void on_complete();
};
 */

}}


#include "observable.hpp"
