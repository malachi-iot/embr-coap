#include "coap-dispatcher.h"
#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>

#define COAP_UDP_PORT 5683

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
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 2>
};


// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
BlockingEncoder* global_encoder;

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
bool done_encoding;

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
        if(context->token())
            global_encoder->token(*context->token());

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
};
