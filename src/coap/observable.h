#pragma once

#include "decoder/subject.h"
#include "encoder.h"
#include "experimental-observer.h"
#include "exp/datapump-observer.h"
#include "coap/platform.h"
#include <estd/vector.h>

namespace moducom { namespace  coap {

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


namespace experimental {

// specialized code to handle re-queuing outgoing messages by modifying their headers
// so that we can blast through 'j' number of notification targets but only allocating
// one or two netbufs
template<class TDataPump,
         class TIterator,
         class TNetBuf = typename TDataPump::netbuf_t,
         class TAddr = typename TDataPump::addr_t>
class ObservableMetaSession :
        public IDataPumpObserver<TNetBuf, TAddr>
{
    typedef TNetBuf netbuf_t;

    TDataPump& datapump;
    TIterator it;
    TIterator it_end;       // TODO: Would really like to optimize this out

    virtual void on_message_transmitting(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
    {

    }

    virtual void on_message_transmitted(TNetBuf* netbuf, const TAddr& addr) OVERRIDE
    {
        // When a observable message is fired off, we want to then:
        // 1. notice if there are any more in the observable list which want to be notified
        // 2. if so, let 'n' = new target to notify
        // 3. take this incoming netbuf and requeue it for send, modifying mid/token in coap
        //    to match 'n' original mid/token which it registered with, and of course sending
        //    to 'n' address
        // 4. eventually, we also need to re-encode Option sequence number, though I expect the
        //    importance of that is a small edge case
        if(it != it.end())
        {
            typedef typename TIterator::value_type value_type;
            value_type& v = *it;
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> rewritten_header_and_token;
            netbuf_t& new_netbuf = rewritten_header_and_token.netbuf();
#else
            netbuf_t new_netbuf = new netbuf_t;

            NetBufEncoder<netbuf_t&> rewritten_header_and_token(*new_netbuf);
#endif
            NetBufDecoder<netbuf_t&> decoder(*netbuf);

            Header header = decoder.header();

            header.token_length(it.token.length());

            rewritten_header_and_token.header(header);
            rewritten_header_and_token.token(it.token());

#ifdef FEATURE_MCCOAP_DISCRETE_OBSERVER_SEQUENCE
#error Discrete copied observe sequence feature not yet implemented
#endif

            // copy just-sent netbuf to new newbut, skipping the header and token of
            // source (explicitly) and destingation (implicitly)
            coap_netbuf_copy(*netbuf, rewritten_header_and_token.netbuf(), 4 + header.token_length());

            rewritten_header_and_token.netbuf().complete();

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.enqueue_out(std::forward(new_netbuf), it.addr);
#else
            datapump.enqueue_out(*netbuf, it.addr);
#endif

            ++it;
        }

        // if it == end here, then nothing was enqueued, therefore this observer should
        // fall away and not be called again
    }

public:
    ObservableMetaSession(TDataPump& datapump,
                          TIterator it,
                          TIterator it_end) :
        datapump(datapump),
        it(it),
        it_end(it_end)
        {}
};

}


// A registrar represents ONE subject's list of listeners.  By subject
// we generally mean one subscribable URI path
template <class TCollection,
          class TIncomingContext = AddressContext<
              typename TCollection::value_type::addr_t
              >,
          class TRequestContextTraits = incoming_context_traits<TIncomingContext> >
class ObservableRegistrar
{
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

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
        public moducom::coap::ObservableRegistrar<
                estd::layer1::vector<TObservableSession, N> >
{
};

}

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

}}


#include "observable.hpp"
