#include <sys/socket.h>

#include "coap/observable.h"
#include "coap-uripath-dispatcher.h"

#include "coap/decoder-subject.hpp"

#include "main.h"

using namespace moducom::pipeline;
using namespace moducom::coap;
using namespace moducom::coap::experimental;


#ifndef FEATURE_MCCOAP_INLINE_TOKEN
#error "Requires inline token support"
#endif



IDecoderObserver* sensor1_handler(AggregateUriPathObserver::Context& ctx)
{
    // TODO: In here, handle both a subscription request as well as
    // the resource request itself.  TBD whether we need to return 'Observe'
    // option with the (piggybacked) ACK from here
    dispatcher_handler_factory_fn factories[] =
    {
        //[](FactoryDispatcherHandlerContext& c) -> IDecoderObserver*
        //{ return new (c.handler_memory.data()) int; },
        nullptr
    };

    // FIX: Beware, this results in an alloc which does not get freed
    // NOTE: Also have to be very, very careful that the passed in ctx.objstack
    //      reflects the state of ctx.objstack *after* placement new, so must
    //      always pass in by reference/pointer
    return new (ctx) FactoryDispatcherHandler(ctx.context.objstack, ctx.context, factories);
}


IDecoderObserver* sensor2_handler(AggregateUriPathObserver::Context& ctx)
{
    return nullptr;
}


IDecoderObserver* v1_handler(AggregateUriPathObserver::Context& ctx)
{
    typedef AggregateUriPathObserver::fn_t fn_t;
    typedef AggregateUriPathObserver::item_t item_t;

    item_t items[] =
    {
        fn_t::item("sensor1", sensor1_handler),
        fn_t::item("sensor2", sensor2_handler)
    };

    // FIX: Beware, this results in an alloc which does not get freed
    return new (ctx.context.objstack) AggregateUriPathObserver(ctx, items);
}

IDecoderObserver* context_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
    return new (ctx) ContextDispatcherHandler(ctx.incoming_context);
}


IDecoderObserver* v1_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
    typedef AggregateUriPathObserver::fn_t fn_t;
    typedef AggregateUriPathObserver::item_t item_t;

    item_t items[] =
    {
        fn_t::item("v1", v1_handler)
    };

    // FIX: Since objstack implementation bumps up its own m_data, we can safely pass it
    // in for the used memorychunk.  However, this is all clumsy and we should be passing
    // around objstack more universally
    AggregateUriPathObserver* observer = new (ctx)
            AggregateUriPathObserver(ctx.incoming_context, items);

    return observer;
}



static dispatcher_handler_factory_fn root_factories[] =
{
    context_dispatcher,
    v1_dispatcher
};

template <>
size_t service_coap_in(const struct sockaddr_in& address_in, MemoryChunk& inbuf, MemoryChunk& outbuf)
{
    moducom::pipeline::layer1::MemoryChunk<512> buffer;
    ObserverContext incoming_context(buffer);

    FactoryDispatcherHandler dh(buffer, incoming_context, root_factories);
    DecoderSubjectBase<IDecoderObserver&> decoder(dh);

    decoder.dispatch(inbuf);

    return 0;
}


// return 0 always, for now
template <>
size_t service_coap_out(struct sockaddr_in* address_out, MemoryChunk& outbuf)
{
    return 0;
}