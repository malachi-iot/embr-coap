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



IDispatcherHandler* sensor1_handler(AggregateUriPathObserver::Context& ctx)
{
    return nullptr;
}


IDispatcherHandler* sensor2_handler(AggregateUriPathObserver::Context& ctx)
{
    return nullptr;
}


IDispatcherHandler* v1_handler(AggregateUriPathObserver::Context& ctx)
{
    typedef AggregateUriPathObserver::fn_t fn_t;
    typedef AggregateUriPathObserver::item_t item_t;

    item_t items[] =
    {
        fn_t::item("sensor1", sensor1_handler),
        fn_t::item("sensor2", sensor2_handler)
    };

    return new (ctx.objstack) AggregateUriPathObserver(ctx, items);
}

IDispatcherHandler* context_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx.incoming_context);
}


IDispatcherHandler* v1_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
    moducom::dynamic::ObjStack objstack(ctx.handler_memory);
    typedef AggregateUriPathObserver::fn_t fn_t;
    typedef AggregateUriPathObserver::item_t item_t;

    item_t items[] =
    {
        fn_t::item("v1", v1_handler)
    };

    // FIX: Since objstack implementation bumps up its own m_data, we can safely pass it
    // in for the used memorychunk.  However, this is all clumsy and we should be passing
    // around objstack more universally
    AggregateUriPathObserver* observer = new (objstack)
            AggregateUriPathObserver(objstack, ctx.incoming_context, items);

    return observer;
}



dispatcher_handler_factory_fn root_factories[] =
{
    context_dispatcher,
    v1_dispatcher,
    // FIX: Enabling this causes a crash in the on_option area
    //fallthrough_404
};

template <>
size_t service_coap_in(const struct sockaddr_in& address_in, MemoryChunk& inbuf, MemoryChunk& outbuf)
{
    moducom::pipeline::layer1::MemoryChunk<512> buffer;
    IncomingContext incoming_context;

    FactoryDispatcherHandler dh(buffer, incoming_context, root_factories);
    DecoderSubjectBase<IDispatcherHandler&> decoder(dh);

    decoder.dispatch(inbuf);

    //IncomingContext context;
    //DecoderSubjectBase<ObservableOptionObserverBase> decoder(context);

    //decoder.dispatch(inbuf);

    return 0;
}


// return 0 always, for now
template <>
size_t service_coap_out(struct sockaddr_in* address_out, MemoryChunk& outbuf)
{
    return 0;
}
