#pragma once

#include "decoder-subject.h"
#include "experimental-observer.h"

namespace moducom { namespace  coap {

// Not seeing anything specific in RFC7641 mandating or denying the use of CON or NON
// messaging for observables, thereby implying we don't need to track what flavor
// we are in this context either
struct ObservableContext
{
    // increases with each observe notification.  Technically only needs to be 24 bits
    // so if we do need flags, we can make this into a bit field.  Also, as suggested
    // in RFC7641, we might do without a sequence# altogether and instead use a mangled
    // version of system timestamp.  In that case, holding on to sequence# here might
    // be a waste.  What does not seem to be specified at all is how to
    // handle rollover/overflow conditions.
    uint32_t sequence;

    layer1::Token token;
};


template <class TEnumeration, class TAddr = uint8_t[4]>
class ObservableRegistrar
{
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

    TEnumeration registrations;

public:
    // When evaluating a registration or deregistration, utilize this context
    class Context
    {
    public:
        // true = is registering
        // false = is deregistering
        const bool is_registering;

        // super-experimental, but we do need this IP(or other type) address during registration
        // either that, or some kinda wacky out of band token<-->address map
        TAddr addr;

        Context(bool is_registering, const TAddr& addr) :
            is_registering(is_registering)
        {
            // FIX: definitely not gonna work for all scenarios
            memcpy(&this->addr, &addr, sizeof(TAddr));
        }
    };

    // when ObservableOptionObserverBase finds a registration or deregistration,
    // call this
    void on_uri_path(Context& context, const ro_chunk_t& chunk, bool last_chunk) {}

    // when ObservableOptionObserverBase completes a registration or deregistration gather
    // call this
    void on_complete(Context& context) {}
};


class ObservableOptionObserverBase : public experimental::MessageObserverBase
{
    typedef experimental::MessageObserverBase base_t;
    // FIX: int is just a dummy value while we experiment
    typedef ObservableRegistrar<int> registrar_t;

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
