#include <sys/socket.h>

#include "coap/observable.h"
#include "coap-uripath-dispatcher.h"

#include "coap/decoder-subject.hpp"
#include "coap-dispatcher.hpp"

#include "main.h"

using namespace moducom::pipeline;
using namespace moducom::coap;
using namespace moducom::coap::experimental;


#ifndef FEATURE_MCCOAP_INLINE_TOKEN
#error "Requires inline token support"
#endif

typedef ObserverContext request_context_t;


IDecoderObserver<request_context_t>* sensor1_handler(
        AggregateUriPathObserver<request_context_t>::Context& ctx)
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
    return new (ctx.context) FactoryDispatcherHandler<request_context_t>(ctx.context, factories);
}


IDecoderObserver<request_context_t>* sensor2_handler(
        AggregateUriPathObserver<request_context_t>::Context& ctx)
{
    return nullptr;
}


IDecoderObserver<request_context_t>* v1_handler(
        AggregateUriPathObserver<request_context_t>::Context& ctx)
{
    typedef AggregateUriPathObserver<request_context_t>::fn_t fn_t;
    typedef AggregateUriPathObserver<request_context_t>::item_t item_t;

    item_t items[] =
    {
        fn_t::item("sensor1", sensor1_handler),
        fn_t::item("sensor2", sensor2_handler)
    };

    // FIX: Beware, this results in an alloc which does not get freed
    return new (ctx.context) AggregateUriPathObserver<request_context_t>(ctx, items);
}

IDecoderObserver<request_context_t>* context_dispatcher(request_context_t& ctx)
{
    return new (ctx) ContextDispatcherHandler<request_context_t>(ctx);
}


IDecoderObserver<request_context_t>* v1_dispatcher(request_context_t& ctx)
{
    typedef AggregateUriPathObserver<request_context_t>::fn_t fn_t;
    typedef AggregateUriPathObserver<request_context_t>::item_t item_t;

    item_t items[] =
    {
        fn_t::item("v1", v1_handler)
    };

    // FIX: Since objstack implementation bumps up its own m_data, we can safely pass it
    // in for the used memorychunk.  However, this is all clumsy and we should be passing
    // around objstack more universally
    AggregateUriPathObserver<request_context_t>* observer = new (ctx)
            AggregateUriPathObserver<request_context_t>(ctx, items);

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

    FactoryDispatcherHandler<request_context_t> dh(incoming_context, root_factories);
    DecoderSubjectBase<IDecoderObserver<request_context_t>&> decoder(dh);

    decoder.dispatch(inbuf);

    return 0;
}


// return 0 always, for now
template <>
size_t service_coap_out(struct sockaddr_in* address_out, MemoryChunk& outbuf)
{
    return 0;
}
