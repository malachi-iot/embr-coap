#pragma once

#include "../coap-dispatcher.h"

namespace moducom { namespace coap { namespace experimental {

// use this to make non-virtual versions
// TODO: Do further testing to see how optimal that really is (maybe
// compiler optimizations make unused virtual tables less costly?  If
// so, may not be worth the effort to make a non-virtualized flavor)
template <class TIncomingContext = ObserverContext,
          class TIncomingContextTraits = incoming_context_traits<TIncomingContext> >
class MessageObserverBase :
        public IsInterestedBase,
        public ContextContainer<TIncomingContext, TIncomingContextTraits>
{
protected:
    typedef internal::option_number_t option_number_t;
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

    //FIX:
    // a) undecided if we'll ONLY do this->context(context) version
    // b) SFINAE for is_constructor picks this up even when procected, then gets irritated
    //    that we can't actually call it
    MessageObserverBase(TIncomingContext& context)
    {
        this->context(context);
    }
public:
    MessageObserverBase() {}


    void on_header(const Header& header) {}

    void on_token(const ro_chunk_t& token, bool last_chunk) {}

    void on_option(option_number_t number, uint16_t len) {}

    void on_option(option_number_t number, const ro_chunk_t& chunk, bool last_chunk) {}

    void on_payload(const ro_chunk_t& chunk, bool last_chunk) {}

    // Unlike IDecoderObserver, no penalty in having this one here so have it enabled always
    void on_complete() {}
};

namespace layer5 {

// Experimental code to bring in classes which weren't initially declared virtual
// (perhaps named either layer2 or layer3) into the virtual domain
// Or, similarly, all observers might start life as non-virtual and only become virtual
// when wrapped with this guy
template <class TMessageObserver, class TBase = IMessageObserver>
class IMessageObserverWrapper : public TBase
{
    TMessageObserver message_observer;
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef TBase base_t;
    typedef typename base_t::number_t number_t;

public:
    //IMessageObserverWrapper(const TMessageObserver& message_observer) :
    //    message_observer(message_observer) {}

#ifdef FEATURE_CPP_VARIADIC
    template <typename ... TArgs>
    IMessageObserverWrapper(TArgs...args) :
        message_observer(args...)
    {

    }
#else
    template <class TArg1>
    IMessageObserverWrapper(const TArg1& arg1) :
        message_observer(arg1)
    {

    }
#endif

    virtual void on_header(Header header) OVERRIDE
    {
        message_observer.on_header(header);
    }

    virtual void on_token(const ro_chunk_t& token_part, bool last_chunk) OVERRIDE
    {
        message_observer.on_token(token_part, last_chunk);
    }

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {
        message_observer.on_option(number, length);
    }

    virtual void on_option(number_t number,
                           const ro_chunk_t& option_value_part,
                           bool last_chunk) OVERRIDE
    {
        message_observer.on_option(number, option_value_part, last_chunk);
    }

    virtual void on_payload(const ro_chunk_t& payload_part, bool last_chunk) OVERRIDE
    {
        message_observer.on_payload(payload_part, last_chunk);
    }

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE
    {
        message_observer.on_complete();
    }
#endif

};

}

}}}
