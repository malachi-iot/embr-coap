#include "coap-dispatcher.h"
#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>


using namespace moducom;
using namespace embr::coap;
using namespace embr::coap::experimental;

// NOTE: Not used yet due to lingering kludginess regarding
// done_encoding and global_encoder
template<size_t n> //double f(double (&c)[n]);
static void root_helper(pipeline::MemoryChunk& dispatcherBuffer,
                        pipeline::MemoryChunk& in,
                        pipeline::MemoryChunk& out,
                        dispatcher_handler_factory_fn (&root_factories)[n])
{
    moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(out);
    //embr::coap::experimental::Dispatcher dispatcher;
    embr::coap::experimental::BlockingEncoder encoder(writer);

    ObserverContext incoming_context(dispatcherBuffer);

#ifdef FEATURE_MCCOAP_LEGACY_PREOBJSTACK
    FactoryDispatcherHandler handler(dispatcherBuffer, incoming_context, root_factories);
#else
    FactoryDispatcherHandler handler(incoming_context, root_factories);
#endif
}

IDecoderObserver* context_dispatcher(FactoryDispatcherHandlerContext& ctx);


// NOTE: ALso not used yet because the string-factory template manager still being worked
// on could radically change our preferred way of dispatching uri-path
template <const char* uri_path, dispatcher_handler_factory_fn* factories, int count>
static void root_helper2(pipeline::MemoryChunk& dispatcherBuffer,
                         pipeline::MemoryChunk& in,
                         pipeline::MemoryChunk& out)
{
    dispatcher_handler_factory_fn root_factories[] =
    {
        context_dispatcher,
        // FIX: Right, because FactoryDispatcherHandler don't pass contexts, we start with a new
        // one when we get here - which is not entirely accurate
        uri_plus_factory_dispatcher<uri_path, factories, count>
    };

    root_helper(dispatcherBuffer, in, out, root_factories);
};

