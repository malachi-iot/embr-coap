#pragma once

namespace moducom { namespace coap {

namespace experimental {

template <class TIncomingContext>
inline typename FactoryDispatcherHandler<TIncomingContext>::decoder_observer_t*
    FactoryDispatcherHandler<TIncomingContext>::observer_helper_begin(context_t& context, int i)
{
    State& state = handler_state(i);
    context.state = &state;

    // unless we are *never* interested, have a crack at observing
    if(state.is_never_interested() && state.initialized()) return NULLPTR;

#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
    if(state.reserved) return state.reserved;
#endif

    IDecoderObserver<TIncomingContext>* handler = handler_factories[i](context);
    return handler;
}


template <class TIncomingContext>
inline void FactoryDispatcherHandler<TIncomingContext>::observer_helper_end(context_t& context, FactoryDispatcherHandler::decoder_observer_t* handler)
{
    State& state = *context.state;

    // we query interested after processing, because we must always process
    // *something* to know whether we wish to continue processing it
    state.interested(handler->interested());

    // reserved handlers don't process any additional reserved bytes,
    // nor are they further candidates for chosen, and finally
    // their constructors don't execute here
#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
    if(!state.reserved)
#endif
    {
#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
        if(context.reserve_bytes)
        {
            // be very mindful of reserved handlers.  They consume objstack
            // space and if they go into 'never' interested mode, it's just
            // a waste - attempt rearchitecture for those conditions
            state.reserved = handler;
        }

        this->context.reserve_bytes += context.reserve_bytes;
#endif

        if(state.is_always_interested())
        {
            chosen = handler;
            return; // obviously do NOT destruct the chosen handler
        }

        // placement new demands explicit destructor invocation
        // this is called every time lock step with observer_helper_begin
        // unless using experimental reserved mode OR we are in always-
        // interested mode
        handler->~decoder_observer_t();

        // TODO: Somehow we need an intelligent objstack.free right here
        // which knows correct amount of bytes to free

        // consider solving this by using GNU recommended style
        // described here https://www.gnu.org/software/libc/manual/html_node/Obstacks.html
        // stating that a free operation is more or less an explicit
        // reposition to the specified pointer location.  This is less aligned
        // with traditional allocation techniques (and as such may be harder
        // to enhance our dispatcher/observers with traditional allocation)

        // upside to using GNU-style is a lot less code is involved, including no need
        // for explicit deallocation in virtual destructors.  Something about that
        // feels like almost too much of a shortcut, though technically it is sound
#ifndef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context.objstack.free(handler);
#endif
    }
}


template <class TIncomingContext>
void FactoryDispatcherHandler<TIncomingContext>::on_header(Header header)
{
#ifdef ESP_DEBUG
    printf("\r\nFactoryDispatcherHandler::on_header type=%d, mid=%X",
        header.type(), header.message_id());
#endif

    for(int i = 0; i < handler_factory_count; i++)
    {
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context_t ctx(context, handler_memory());
#else
        context_t ctx(m_context);
#endif

        decoder_observer_t* handler = observer_helper_begin(ctx, i);

        if(handler == NULLPTR) continue;

        handler->on_header(header);

        observer_helper_end(ctx, handler);
    }
}


template <class TIncomingContext>
void FactoryDispatcherHandler<TIncomingContext>::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{
#ifdef ESP_DEBUG
    printf("\r\nFactoryDispatcherHandler::on_token: len=%d", token_part.length());
#endif


    if(chosen)
    {
        chosen->on_token(token_part, last_chunk);
        return;
    }

    // NOTE: Somewhat unlikely that we'd dispatch on token itself, but conceivable
    // may want to split this into a distinct kind of dispatcher, just because dispatching on
    // token feels like an explicitly pre known circumstance i.e. session management feature
    for(int i = 0; i < handler_factory_count; i++)
    {
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context_t ctx(context, handler_memory());
#else
        context_t ctx(m_context);
#endif

        decoder_observer_t* handler = observer_helper_begin(ctx, i);

        if(handler == NULLPTR) continue;

        handler->on_token(token_part, last_chunk);

        observer_helper_end(ctx, handler);
    }
}


template <class TIncomingContext>
void FactoryDispatcherHandler<TIncomingContext>::on_option(number_t number, uint16_t length)
{
    if(chosen != NULLPTR)
    {
        chosen->on_option(number, length);
        return;
    }
}

template <class TIncomingContext>
void FactoryDispatcherHandler<TIncomingContext>::on_option(number_t number,
                                      const pipeline::MemoryChunk::readonly_t& option_value_part,
                                      bool last_chunk)
{
    if(chosen != NULLPTR)
    {
        chosen->on_option(number, option_value_part, last_chunk);
        return;
    }

    for(int i = 0; i < handler_factory_count; i++)
    {
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context_t ctx(context, handler_memory());
#else
        context_t ctx(m_context);
#endif

        decoder_observer_t* handler = observer_helper_begin(ctx, i);

        if(handler == NULLPTR) continue;

        // FIX: This only works for non-chunked processing
        handler->on_option(number, option_value_part.length());
        handler->on_option(number, option_value_part, true);

        observer_helper_end(ctx, handler);
    }
}


template <class TRequestContext>
void FactoryDispatcherHandler<TRequestContext>::on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                                       bool last_chunk)
{
    if(chosen != NULLPTR)
    {
        chosen->on_payload(payload_part, last_chunk);

#ifndef FEATURE_MCCOAP_COMPLETE_OBSERVER
        // NOTE: Keep an eye on this.  Not sure if this is the exact right
        // place for lifecycle management, but should be a good spot
        if(last_chunk)
            chosen->~IDecoderObserver();


        context.incoming_context.objstack.free(chosen);
#endif

        return;
    }

    for(int i = 0; i < handler_factory_count; i++)
    {
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context_t ctx(context, handler_memory());
#else
        context_t ctx(m_context);
#endif

        decoder_observer_t* handler = observer_helper_begin(ctx, i);

        if(handler == NULLPTR) continue;

        handler->on_payload(payload_part, last_chunk);

        observer_helper_end(ctx, handler);
    }
}

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
template <class TRequestContext>
void FactoryDispatcherHandler<TRequestContext>::on_complete()
{
    // For now, everyone receives an on_complete whether they are
    // interested or not
    // Extremely unlikely that we won't have a chosen handler by now
    // (unlikely use case)
    for(int i = 0; i < handler_factory_count; i++)
    {
#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
        context_t ctx(context, handler_memory());
#else
        context_t ctx(m_context);
#endif

        decoder_observer_t* handler = observer_helper_begin(ctx, i);

        if(handler == NULLPTR) continue;

        handler->on_complete();

        observer_helper_end(ctx, handler);
    }
}
#endif


#ifdef FEATURE_MCCOAP_RESERVED_DISPATCHER
void FactoryDispatcherHandler::free_reserved()
{
    // Extremely unlikely that we won't have a chosen handler by now
    // (unlikely use case)
    for(int i = 0; i < handler_factory_count; i++)
    {
        State& state = handler_state(i);

        if(state.reserved)
        {
            state.reserved->~IDispatcherHandler();
        }
    }
}
#endif


template <class TRequestContext>
void ContextDispatcherHandler<TRequestContext>::on_header(Header header)
{
    this->context().header(header);
}


template <class TRequestContext>
void ContextDispatcherHandler<TRequestContext>::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    // NOTE: this won't yet work with chunked
    this->context().token(&token_part);
#else
    // FIX: access already-allocated token manager (aka token pool) and allocate a new
    // token or link to already-allocated token.
    // For now we use very unreliable static to hold this particular incoming token
    static layer2::Token token;

    // NOTE: this won't yet work with chunked
    token.set(token_part.data(), token_part.length());
    context().token(&token);

    // NOTE: Perhaps we don't want to use token pool at this time, and instead keep a full
    // token reference within TokenContext instead.  We can still pool it later in more of
    // a session-token pool area
#endif
}


}

}}
