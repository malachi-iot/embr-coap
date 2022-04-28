#ifndef ESP32
// ESP32 has its own libcoap, so don't try to do this in that case
// (our other includes should bring it in properly)
#include <coap.h>
#else
#endif

#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>

using namespace moducom;
using namespace embr::coap;
using namespace embr::coap::experimental;

constexpr char STR_URI_V1[] = "v1";
constexpr char STR_URI_TEST[] = "test";
constexpr char STR_URI_TEST2[] = "test2";

// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
embr::coap::experimental::BlockingEncoder* global_encoder;

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
bool done;

class TestDispatcherHandler : public DispatcherHandlerBase
{
public:
    virtual void on_header(Header header) override
    {
        // FIX: on_header never getting called, smells like a problem
        // with the is_interested code
        printf("\r\nGot header: token len=%d", header.token_length());
        printf("\r\nGot header: mid=%x", header.message_id());
    }
    

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) override
    {
        char buffer[128]; // because putchar doesn't want to play nice

        int length = payload_part.copy_to(buffer, 127);
        buffer[length] = 0;

        printf("\r\nGot payload: %s", buffer);

        Header outgoing_header;

        process_request(context->header(), &outgoing_header);

        outgoing_header.response_code(Header::Code::Valid);

        global_encoder->header(outgoing_header);

        // TODO: doublecheck to see if we magically update outgoing header TKL
        // I think we do, though even if we don't the previous .raw = .raw
        // SHOULD copy it, assuming our on_header starts getting called again
        global_encoder->token(context->token());

        // just echo back the incoming payload, for now
        // API not ready yet
        // TODO: Don't like the char* version, so expect that
        // to go away
        global_encoder->payload(buffer);

        done = true;
    }
};

extern dispatcher_handler_factory_fn v1_factories[];

embr::coap::experimental::IDispatcherHandler* context_dispatcher(
    embr::coap::experimental::FactoryDispatcherHandlerContext& ctx)
{
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx.incoming_context);
}

dispatcher_handler_factory_fn root_factories[] =
{
    context_dispatcher,
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 2>
};

// FIX: Kinda-sorta works, TestDispatcherHandler is *always* run if /v1 appears
dispatcher_handler_factory_fn v1_factories[] = 
{
    uri_plus_observer_dispatcher<STR_URI_TEST, TestDispatcherHandler>,
    uri_plus_observer_dispatcher<STR_URI_TEST2, TestDispatcherHandler>
};

