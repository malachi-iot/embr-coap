#pragma once

#include "decoder-subject.h"
#include "experimental-observer.h"

namespace moducom { namespace  coap {

// Not seeing anything specific in RFC7641 mandating or denying the use of CON or NON
// messaging for observables, thereby implying we don't need to track what flavor
// we are in this context either
struct ObservableSession
{
    // increases with each observe notification.  Technically only needs to be 24 bits
    // so if we do need flags, we can make this into a bit field.  Also, as suggested
    // in RFC7641, we might do without a sequence# altogether and instead use a mangled
    // version of system timestamp.  In that case, holding on to sequence# here might
    // be a waste.  What does not seem to be specified at all is how to
    // handle rollover/overflow conditions.
    uint32_t sequence;

    // we are the subject, so addr is address of observer - the one
    // who subscribed to us
    IncomingContext::addr_t addr;

    // token is one presented during initial subscription
    layer2::Token token;
};


// A registrar represents ONE subject's list of listeners.  By subject
// we generally mean one subscribable URI path
template <class TCollection>
class ObservableRegistrar
{
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

    // Enumeration shall be of type ObservableSession, or something similar to it
    TCollection registrations;

    typedef typename TCollection::value_type observable_session_t;

public:
    // When evaluating a registration or deregistration, utilize this context
    class Context
    {
    public:
        // true = is registering
        // false = is deregistering
        const bool is_registering;

        // will be carrying address info
        const IncomingContext& incomingContext;

        Context(const IncomingContext& incomingContext, bool is_registering) :
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
        ObservableSession test;

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

class ObservableOptionObserverBase : public experimental::MessageObserverBase
{
    typedef experimental::MessageObserverBase base_t;

    // FIX: Once LinkedListPool is operational use that for our allocator
    // or at least have it as a default template parameter for ObservableOptionObserverBase itself
    typedef estd::forward_list<
            ObservableSession,
                estd::experimental::ValueNode<
                    ObservableSession,
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
    ObservableOptionObserverBase(IncomingContext& context) :
        base_t(context)
    {}

    void on_option(option_number_t number, uint16_t len) {}

    void on_option(option_number_t number, const ro_chunk_t& chunk, bool last_chunk);

    void on_complete();
};

}}
