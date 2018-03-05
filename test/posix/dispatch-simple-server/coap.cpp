#include "coap-dispatcher.h"
#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>
#include "experimental.h"

using namespace moducom;
using namespace moducom::coap;
using namespace moducom::coap::experimental;

constexpr char STR_URI_V1[] = "v1";
constexpr char STR_URI_TEST[] = "test";
constexpr char STR_URI_TEST2[] = "test2";

extern dispatcher_handler_factory_fn v1_factories[];

IDispatcherHandler* context_dispatcher(FactoryDispatcherHandlerContext& ctx)
{
    typedef moducom::dynamic::OutOfBandPool<moducom::coap::layer2::Token> token_pool_t;

    static moducom::coap::layer2::Token _dummy[4];
    static token_pool_t dummy(_dummy, 4);

    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx.incoming_context, dummy);
}

dispatcher_handler_factory_fn root_factories[] =
{
    context_dispatcher,
    // FIX: Right, because FactoryDispatcherHandler don't pass contexts, we start with a new
    // one when we get here - which is not entirely accurate
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 1>
};



// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
BlockingEncoder* global_encoder;

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
bool done_encoding;


// FIX: experimental naming
void handle_response(BlockingEncoder* encoder, IncomingContext* context,
                     Header::Code::Codes code = Header::Code::Valid)
{
    Header outgoing_header = create_response(context->header(), code);

    encoder->header(outgoing_header);

    // TODO: doublecheck to see if we magically update outgoing header TKL
    // I think we do, though even if we don't the previous .raw = .raw
    // SHOULD copy it, assuming our on_header starts getting called again
    if(context->token())
        encoder->token(*context->token());
}

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
        handle_response(global_encoder, context, Header::Code::Valid);

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


// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;


size_t service_coap_in(pipeline::MemoryChunk& in, pipeline::MemoryChunk& outbuf)
{
    moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
    // NOTE: Consider renaming dispatcher to something more like DecodeAndObserve, though
    // form is wrong.  Proper form would be more like DecoderThenObserver or DecodeAndObserver
    // but those sound bad.  Perhaps MessageObservable?  DecodeMessageObservable? yuck.
    // ObservableMessage?  Nope... all bad, every one.
    moducom::coap::experimental::Dispatcher dispatcher;
    moducom::coap::experimental::BlockingEncoder encoder(writer);
    IncomingContext incoming_context;

    FactoryDispatcherHandler handler(dispatcherBuffer, incoming_context, root_factories);
    //TestDispatcherHandler handler;

    // FIX: fix this gruesomeness
    global_encoder = &encoder;

    dispatcher.head(&handler);
    dispatcher.dispatch(in, true);

    size_t send_bytes = writer.length_experimental();

    // Does nothing at the moment
    root_helper2<STR_URI_V1, v1_factories, 1>(dispatcherBuffer, in, outbuf);

    return send_bytes;
}
