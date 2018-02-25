#include <coap.h>
#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>

#include "lwip/api.hpp"

#define COAP_UDP_PORT 5683

using namespace moducom;
using namespace moducom::coap;
using namespace moducom::coap::experimental;

// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;
static pipeline::layer3::MemoryChunk<256> outbuf;

constexpr char STR_URI_V1[] = "v1";
constexpr char STR_URI_TEST[] = "test";
constexpr char STR_URI_TEST2[] = "test2";

// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
moducom::coap::experimental::BlockingEncoder* global_encoder;

class TestDispatcherHandler : public DispatcherHandlerBase
{
    //experimental::BlockingEncoder& encoder;
    Header outgoing_header;

public:
    //TestDispatcherHandler(experimental::BlockingEncoder& encoder)
    //    encoder(encoder) {}
    virtual void on_header(Header header) OVERRIDE
    {
        // FIX: This is very preliminary code.  What we really should do
        // is properly initialize a new header, and then explicitly copy MID
        outgoing_header.raw = header.raw;

        // FIX: This assumes a CON request
        outgoing_header.type(Header::Acknowledgement);

        // FIX: Assumes a GET operation
    }
    

    void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) override
    {
        char buffer[128]; // because putchar doesn't want to play nice

        int length = payload_part.copy_to(buffer, 127);
        buffer[length] = 0;

        printf("\r\nGot payload: %s", buffer);

        outgoing_header.response_code(Header::Code::Valid);

        global_encoder->header(outgoing_header);

        // just echo back the incoming payload, for now
        // API not ready yet
        // TODO: Don't like the char* version, so expect that
        // to go away
        global_encoder->payload(buffer);
    }
};

extern dispatcher_handler_factory_fn v1_factories[];

dispatcher_handler_factory_fn root_factories[] =
{
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 1>
};

// FIX: Kinda-sorta works, TestDispatcherHandler is *always* run if /v1 appears
dispatcher_handler_factory_fn v1_factories[] = 
{
    uri_plus_observer_dispatcher<STR_URI_TEST, TestDispatcherHandler>,
    uri_plus_observer_dispatcher<STR_URI_TEST2, TestDispatcherHandler>
};




extern "C" void coap_daemon(void *pvParameters)
{
    lwip::Netconn conn(NETCONN_UDP);
    lwip::Netbuf buf;

    conn.bind(nullptr, COAP_UDP_PORT);

    for(;;)
    {
        printf("\r\nListening for COAP data");

        conn.recv(buf);

        void* data;
        uint16_t len;

        buf.data(&data, &len);

        pipeline::MemoryChunk chunk((uint8_t*)data, len);

        FactoryDispatcherHandler fdh(dispatcherBuffer, root_factories, 1);
        Dispatcher dispatcher;
        pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
        moducom::coap::experimental::BlockingEncoder encoder(writer);

        global_encoder = &encoder;
        printf("\r\nGot COAP data: %d", len);

        dispatcher.head(&fdh);
        dispatcher.dispatch(chunk);

        buf.free();
    }
}
