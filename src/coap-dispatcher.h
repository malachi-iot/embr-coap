//
// Created by Malachi Burke on 12/26/17.
//

#ifndef MC_COAP_TEST_COAP_DISPATCHER_H
#define MC_COAP_TEST_COAP_DISPATCHER_H

#include "coap/decoder.h"
//#include "mc/linkedlist.h"
#include "mc/array-helper.h"
#include "coap/context.h"
#include "coap/decoder/observer.h"

namespace embr { namespace coap {



// At this point, only experimental because I keep futzing with the names - don't like IDispatcherHandler
// Perhaps IMessageHandler - finally - is the right term?  But IDispatcher + interested() call *do* go
// together...
namespace experimental {

// An in-place new is expected
typedef IDecoderObserver<ObserverContext>* (*dispatcher_handler_factory_fn)(ObserverContext&);


/*
struct ShimDispatcherHandlerTraits
{
    static bool evaluate_header() { return true; }
    static bool evaluate_token() { return true; }
    static bool evaluate_payload() { return true; }
}; */



// Utilizes a list of IDispatcherHandler factories to create then
// evaluate incoming messages for interest.  NOTE: Each created
// IDispatcherHandler is immediately destructed if it does not
// express interest, so until one is chosen, it is considered stateless
// this also means that if any option (dispatch-on) data is chunked, it
// will create an awkwardness so likely that needs to be cached up before
// hand, if necessary
//template <class TTraits = ShimDispatcherHandlerTraits>
// Not doing TTraits just yet because that would demand a transition to an .hpp file,
// which I am not against but more work than appropriate right now
//
// Presently this class also elects the first IDispatcherHandler which expresses
// an "Always" interested as the one and only one to further process the message
// clearly this isn't a system-wide desirable behavior, so be warned.  We do this
// because the memory management scheme only supports one truly active IDispatcherHandler
template <class TRequestContext = ObserverContext>
class FactoryDispatcherHandler : public IDecoderObserver<TRequestContext>
{
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
    pipeline::MemoryChunk _handler_memory;
#endif

    const dispatcher_handler_factory_fn* handler_factories;
    const int handler_factory_count;

public:
    typedef IDecoderObserver<TRequestContext> base_t;
    typedef base_t decoder_observer_t;
    typedef TRequestContext request_context_t;
    typedef typename base_t::number_t number_t;
    typedef typename base_t::interested_t interested_t;

private:

    // TODO: Make context & incoming_context something that is passed in
    // even to FactoryDispatcherHandler
    //IncomingContext& incoming_context;

    class State : public IsInterestedBase
    {
        // FIX: optimize this out
        bool state_initialized;

    public:
        // FIX: optimize this somehow too
        // utilize this for reserved_bytes objstack-allocated
        // (or perhaps other allocation method) permanent item
#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
        decoder_observer_t* reserved;
#endif

        bool initialized() const { return state_initialized; }

        void interested(InterestedEnum value)
        {
            state_initialized = true;
            IsInterestedBase::interested(value);
        }

        State() :
#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
            reserved(NULLPTR),
#endif
            state_initialized(false)
        {}
    };

    // Context local to FactoryDispatcherHandler::on_xxx calls, carries around
    // local state for convenience
    // FIX: copying request_context_t likely not desirable - probably now
    // we want a reference to it
    struct Context : public request_context_t
    {
        State* state;

#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        Context(FactoryDispatcherHandlerContext& ctx, const pipeline::MemoryChunk& chunk)
            : FactoryDispatcherHandlerContext(ctx.incoming_context, chunk) {}
#else
        Context(request_context_t& ctx) :
                request_context_t(ctx) {}
#endif
    };

    // temporary state context during dig
    typedef Context context_t;

    request_context_t m_context;


//#define FEATURE_FDH_FANCYMEM

#ifdef FEATURE_FDH_FANCYMEM
    // where candidate handlers get their semi-objstack from
    pipeline::MemoryChunk handler_memory() const
    {
        // skip past what we use for handler states
        return _handler_memory.remainder(handler_states_size()
#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
                                         + context.reserve_bytes
#endif
                                         );
    }

    State* handler_states() const
    {
        return reinterpret_cast<State*>(_handler_memory.data());
    }

#else
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
    const pipeline::MemoryChunk& handler_memory() const
    {
        return _handler_memory;
    }
#else
#endif

    State _handler_states[10];

    State* handler_states() { return _handler_states; }

#endif

    // TODO: Redundant for simple array-pooled version, but we'll clean that up later
    void init_states()
    {
        moducom::experimental::ArrayHelperBase<State>::construct(handler_states(), handler_factory_count);
    }

    decoder_observer_t* chosen;

    size_t handler_states_size() const
    {
        return handler_factory_count * sizeof(State);
    }

    State& handler_state(int index) { return handler_states()[index]; }

    decoder_observer_t* observer_helper_begin(context_t& context, int i);

    void observer_helper_end(context_t& context, decoder_observer_t* handler);

    void free_reserved();

public:
    FactoryDispatcherHandler(
            request_context_t& incoming_context,
            dispatcher_handler_factory_fn* handler_factories,
            int handler_factory_count)

            :
             handler_factories(handler_factories),
             handler_factory_count(handler_factory_count),
             m_context(incoming_context),
             chosen(NULLPTR)
    {
        init_states();
    }


    template<size_t n>
    FactoryDispatcherHandler(
                             request_context_t& incoming_context,
                             dispatcher_handler_factory_fn (&handler_factories)[n]
    )
            :
             handler_factories(handler_factories),
             handler_factory_count(n),
             m_context(incoming_context),
             chosen(NULLPTR)

    {
        init_states();
    }

    virtual void context(request_context_t& context) OVERRIDE
    {
        new (&m_context) request_context_t(context);
    }

    virtual void on_header(Header header) OVERRIDE;

    // get called repeatedly until all portion of token is provided
    // Not called if header reports a token length of 0
    virtual void on_token(const pipeline::MemoryChunk::readonly_t& token_part,
                          bool last_chunk) OVERRIDE;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) OVERRIDE;

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(number_t number,
                           const pipeline::MemoryChunk::readonly_t& option_value_part,
                           bool last_chunk) OVERRIDE;

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                            bool last_chunk) OVERRIDE;

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE;
#endif

    virtual interested_t interested() const OVERRIDE
    {
        return chosen == NULLPTR ?
               IsInterestedBase::Currently : IsInterestedBase::Always;
    }
};



// Looks for header and saves it in IncomingContext
// Looks for a token and if it finds one, registers it with a token pool and saves it in TRequestContext
template <class TRequestContext = ObserverContext>
class ContextDispatcherHandler : public DecoderObserverBase<TRequestContext>
{
    typedef IsInterestedBase::InterestedEnum interested_t;
    typedef embr::coap::layer2::Token token_t;
    typedef DecoderObserverBase<TRequestContext> base_t;

protected:
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

public:
    typedef typename base_t::context_t context_t;
    typedef typename base_t::context_traits_t request_context_traits;

private:

    // NOTE: We aren't using a ref or ptr here as token_pool_t
    // is a relatively inexpensive object, all const, AND
    // could be a wrapper around a regular fixed-array pool
    typedef dynamic::OutOfBandPool<token_t> token_pool_t;

#ifndef FEATURE_MCCOAP_INLINE_TOKEN
    // TODO: Need a special "PoolToken" to wrap around layer2::Token
    // to provide timestamp and pool-handling alloc/dealloc (though
    // the latter *COULD* be provided with a specialized trait, and
    // perhaps should be)
    token_pool_t token_pool;
#endif

public:
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    ContextDispatcherHandler(context_t& context)
#else
    ContextDispatcherHandler(context_t& context,
                             const token_pool_t& token_pool
    ) : token_pool(token_pool)
#endif
    {
        base_t::context(context);
    }

    virtual void on_header(Header header) OVERRIDE;

    virtual void on_token(const ro_chunk_t& token_part,
                          bool last_chunk) OVERRIDE;

    virtual interested_t interested() const OVERRIDE
    {
        const context_t& c = this->context();

        // If we haven't received a header yet, we're still interested
        if(!c.have_header()) return base_t::Currently;

        // If we have a header, are we looking for a token?
        if(c.header().token_length() > 0)
        {
            // FIX: Not compatible with FEATURE_MCCOAP_INLINE_TOKEN
            // since when that feature is enabled, context.token()
            // is ALWAYS not null when tkl > 0
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
            return c.token_present() ? base_t::Never : base_t::Currently;
#else
            // If we are looking for but dont' have a token,
            // we are still interested.  Otherwise, done
            return c.token() ? Never : Currently;
#endif
        }

        // we have a header and aren't looking for a token,
        // so we are never interested anymore
        return base_t::Never;
    }
};



// experimental and probably not so useful.  Rather, we'd toss a TMessageObserver
// in the chain which actually picks up the token straight out of TRequestContext
template <class TTokenObserver, class TRequestContext = ObserverContext>
class TokenSubjectRelayObserver : public ContextDispatcherHandler<TRequestContext>
{
    TTokenObserver& token_observer;
    typedef ContextDispatcherHandler<TRequestContext> base_t;
    typedef typename base_t::ro_chunk_t ro_chunk_t;

public:
    TokenSubjectRelayObserver(TRequestContext& c) : base_t(c) {}

    virtual void on_token(const ro_chunk_t& token, bool last_chunk) OVERRIDE
    {
        base_t::on_token(token, last_chunk);

        // FIX: accomodate for chunking
        token_observer.on_token(token);
    }
};

}

}}

inline void* operator new(size_t sz, embr::coap::ObjStackContext& ctx)
{
    return ctx.objstack.alloc(sz);
}

#include "coap-dispatcher.hpp"

#endif //MC_COAP_TEST_COAP_DISPATCHER_H
