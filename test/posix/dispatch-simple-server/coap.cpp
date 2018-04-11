#include "coap-dispatcher.h"
#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>
#include <coap/decoder-subject.hpp>
#include "experimental.h"
#include "main.h"

using namespace moducom;
using namespace moducom::coap;
using namespace moducom::coap::experimental;

constexpr char STR_URI_V1[] = "v1";
constexpr char STR_URI_TEST[] = "test";
constexpr char STR_URI_TEST2[] = "test2";

extern dispatcher_handler_factory_fn v1_factories[];
extern AggregateUriPathObserver::item_t new_v1_factories[];

// FIX: experimental naming
void issue_response(BlockingEncoder* encoder, IncomingContext* context,
                    Header::Code::Codes code = Header::Code::Valid)
{
    Header outgoing_header = create_response(context->header(), code);

    encoder->header(outgoing_header);

    if(context->token_present())
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
        encoder->token(context->token());
#else
    encoder->token(*context->token());
#endif
}


IDecoderObserver* context_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx.incoming_context);
#else
    typedef moducom::dynamic::OutOfBandPool<moducom::coap::layer2::Token> token_pool_t;

    static moducom::coap::layer2::Token _dummy[4];
    static token_pool_t dummy(_dummy, 4);

    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx.incoming_context, dummy);
#endif
}

// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
BlockingEncoder* global_encoder;

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
bool done_encoding;


// FIX: Doesn't work yet, just a placeholder for when it does
class FallThroughHandler : public DispatcherHandlerBase
{
public:
    FallThroughHandler(IncomingContext& context)
    {
        set_context(context);
    }

    // FIX: Though the signatures are there, the Dispatch/Decoder does not invoke it
    // yet, therefore this feature is still dormant
#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE
    {
        // FIX: Need a better way to ascertain that request was not serviced,
        // but for now this will do
        if(done_encoding)
        {
            issue_response(global_encoder, context, Header::Code::NotFound);
        }
    }
#endif
};

IDecoderObserver* fallthrough_404(FactoryDispatcherHandlerContext& ctx)
{
    return new (ctx.handler_memory.data()) FallThroughHandler(ctx.incoming_context);
}


IDecoderObserver* new_v1_factory(FactoryDispatcherHandlerContext& ctx)
{
    dynamic::ObjStack objstack(ctx.handler_memory);

    // Have to do pre-alloc like this because of how remainder_chunk is utilized
    void* aggregateObserverMemory = objstack.alloc(sizeof(AggregateUriPathObserver));
    void* singleObserverMemory = objstack.alloc(sizeof(SingleUriPathObserver));;
    pipeline::MemoryChunk remainder_chunk = objstack;

    // FIX: For some reason it doesn't template-discover array size of new_v1_factories
    // probably has to do with our forward-extern up above
    auto aggregateObserver = new (aggregateObserverMemory)
            AggregateUriPathObserver(
                remainder_chunk,
                ctx.incoming_context,
                new_v1_factories, 1);

    //aggregateObserver->set_context(ctx.incoming_context);

    auto singleObserver = new (singleObserverMemory)
            SingleUriPathObserver("v1", *aggregateObserver);

    singleObserver->set_context(ctx.incoming_context);

    // FIX: Do clean this up, but wait until after objstack is in place
    // AND after naming is better (then we can more sensibly fuse these two
    // together)
    return singleObserver;
}

// OLD and NEW both seem to suffer from everything passing through too much, something
// maybe about how FactoryDispatcherHandler does things.  Though OLD only does this
// when not giving a URI at all (not well tested for NEW)
//#define OLD

dispatcher_handler_factory_fn root_factories[] =
{
    context_dispatcher,
#ifdef OLD
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 1>,
#else
    // botched because new_v1_factory doesn't actually account for STR_URI_V1
    // need basically the "old" UriPathDispatcherHandler to glue "v1" to
    // the UriDispatcherHandler
    new_v1_factory,
#endif
    // FIX: Enabling this causes a crash in the on_option area
    //fallthrough_404
};



class TestDispatcherHandler : public DispatcherHandlerBase
{
public:
    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                            bool last_chunk) override
    {
        char buffer[128]; // because putchar doesn't want to play nice

        int length = payload_part.copy_to(buffer, 127);
        buffer[length] = 0;

        printf("\r\nGot payload: %s\r\n", buffer);

        // NOTE: It seems quite likely context will soon hold encoder in some form
        // Seeing as encoders themselves are still in flux (they need to be locked down first)
        // we aren't committing context to any encoders/encoder architecture just yet
        issue_response(global_encoder, context, Header::Code::Valid);

        // just echo back the incoming payload, for now
        // API not ready yet
        // TODO: Don't like the char* version, so expect that
        // to go away
        global_encoder->payload(buffer);

        done_encoding = true;
    }
};

dispatcher_handler_factory_fn v1_factories[] =
{
    uri_plus_observer_dispatcher<STR_URI_TEST, TestDispatcherHandler>
};


AggregateUriPathObserver::item_t new_v1_factories[] =
{
    AggregateUriPathObserver::fn_t::item(STR_URI_TEST,
                                     [](AggregateUriPathObserver::Context& c) -> IDecoderObserver*
     {
         auto observer = new (c.objstack) TestDispatcherHandler;

         observer->set_context(c.context);

         return observer;
     }),
    AggregateUriPathObserver::fn_t::item_experimental<TestDispatcherHandler>(STR_URI_TEST2)
};


// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;



template <>
size_t service_coap_in(const struct sockaddr_in& addr, pipeline::MemoryChunk& in, pipeline::MemoryChunk& outbuf)
{
    moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
    // NOTE: Consider renaming dispatcher to something more like DecodeAndObserve, though
    // form is wrong.  Proper form would be more like DecoderThenObserver or DecodeAndObserver
    // but those sound bad.  Perhaps MessageObservable?  DecodeMessageObservable? yuck.
    // ObservableMessage?  Nope... all bad, every one.
#ifdef FEATURE_MCCOAP_LEGACY_DISPATCHER
    moducom::coap::experimental::Dispatcher dispatcher;
#endif
    moducom::coap::experimental::BlockingEncoder encoder(writer);
    IncomingContext incoming_context;

    FactoryDispatcherHandler handler(dispatcherBuffer, incoming_context, root_factories);
    //TestDispatcherHandler handler;

    // FIX: fix this gruesomeness
    global_encoder = &encoder;

#ifdef FEATURE_MCCOAP_LEGACY_DISPATCHER
    dispatcher.head(&handler);
    dispatcher.dispatch(in, true);
#else
    DecoderSubjectBase<IDecoderObserver&> decoder_subject(handler);

    decoder_subject.dispatch(in);
#endif

    size_t send_bytes = writer.length_experimental();

    if(send_bytes == 0)
    {
        // FIX: Need a better way to ascertain that we're handling a bad/unhandled response
        // but for now this will do
        issue_response(&encoder, &incoming_context, Header::Code::NotFound);
        send_bytes = writer.length_experimental();
    }

    // Does nothing at the moment
    root_helper2<STR_URI_V1, v1_factories, 1>(dispatcherBuffer, in, outbuf);

    return send_bytes;
}

template <>
size_t service_coap_out(struct sockaddr_in* addr, pipeline::MemoryChunk& outbuf)
{
    return 0;
}
