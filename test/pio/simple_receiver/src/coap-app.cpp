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

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
bool done;

class TestDispatcherHandler : public DispatcherHandlerBase
{
    //experimental::BlockingEncoder& encoder;
    Header outgoing_header;

public:
    //TestDispatcherHandler(experimental::BlockingEncoder& encoder)
    //    encoder(encoder) {}
    virtual void on_header(Header header) override
    {
        // FIX: This is very preliminary code.  What we really should do
        // is properly initialize a new header, and then explicitly copy MID
        outgoing_header.raw = header.raw;

        // FIX: on_header never getting called, smells like a problem
        // with the is_interested code
        printf("\r\nGot header: token len=%d", header.token_length());
        printf("\r\nGot header: mid=%x", header.message_id());

        // FIX: This assumes a CON request
        outgoing_header.type(Header::Acknowledgement);

        // FIX: Assumes a GET operation
    }
    

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) override
    {
        char buffer[128]; // because putchar doesn't want to play nice

        int length = payload_part.copy_to(buffer, 127);
        buffer[length] = 0;

        printf("\r\nGot payload: %s", buffer);

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

        done = true;
    }
};

extern dispatcher_handler_factory_fn v1_factories[];

dispatcher_handler_factory_fn root_factories[] =
{
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 2>
};

// FIX: Kinda-sorta works, TestDispatcherHandler is *always* run if /v1 appears
dispatcher_handler_factory_fn v1_factories[] = 
{
    uri_plus_observer_dispatcher<STR_URI_TEST, TestDispatcherHandler>,
    uri_plus_observer_dispatcher<STR_URI_TEST2, TestDispatcherHandler>
};


void dump(uint8_t* bytes, int count)
{
    for(int i = 0; i < count; i++)
    {
        printf("%02X ", bytes[i]);
    }
}

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
        ip_addr_t* from_ip = buf.fromAddr();
        uint16_t from_port = buf.fromPort();

        // acquire pointers that buf uses
        buf.data(&data, &len);

        // track them also in memory chunk
        pipeline::MemoryChunk chunk((uint8_t*)data, len);

        FactoryDispatcherHandler fdh(dispatcherBuffer, root_factories, 1);
        Dispatcher dispatcher;
        pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
        moducom::coap::experimental::BlockingEncoder encoder(writer);

        global_encoder = &encoder;
        printf("\r\nGot COAP data: %d", len);

        done = false;

        printf("\r\nRAW: ");

        dump((uint8_t*)data, len);

        dispatcher.head(&fdh);
        dispatcher.dispatch(chunk);

        if(done)
        {
            //lwip::Netbuf buf_out(outbuf.data(), outbuf.length());
            lwip::Netbuf buf_out;

            // doing this because netbuf.ref seems to be bugged
            // for esp8266
            buf_out.alloc(writer.length_experimental());
            buf_out.data(&data, &len);
            memcpy(data, outbuf.data(), len);

            //from_port = COAP_UDP_PORT;
            
            printf("\r\nResponding: %d to %d.%d.%d.%d:%d", len,
                ip4_addr1_16(from_ip),
                ip4_addr2_16(from_ip),
                ip4_addr3_16(from_ip),
                ip4_addr4_16(from_ip),
                from_port);

            printf("\r\nRETURN RAW: ");

            dump((uint8_t*)data, len);

            conn.sendTo(buf_out, from_ip, from_port);

            buf_out.free();
        }

        // this frees only the meta part of buf.  May want a template flag
        // to signal auto free behavior
        buf.free();
    }
}
