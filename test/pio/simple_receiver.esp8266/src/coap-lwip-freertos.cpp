#include "lwip/api.hpp"

#include <coap-uripath-dispatcher.h>
#include <coap-encoder.h>

using namespace moducom;
using namespace embr::coap;
using namespace embr::coap::experimental;

// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;
static pipeline::layer3::MemoryChunk<256> outbuf;

// FIX: We'd much prefer to pass this via a context
// TODO: Make a new kind of encoder, the normal-simple-case
// one which just dumps to an existing buffer without all the
// fancy IPipline/IWriter involvement
extern embr::coap::experimental::BlockingEncoder* global_encoder;

// FIX: This should be embedded either in encoder or elsewhere
// signals that we have a response to send
extern bool done;

#define COAP_UDP_PORT 5683

void dump(uint8_t* bytes, int count)
{
    for(int i = 0; i < count; i++)
    {
        printf("%02X ", bytes[i]);
    }
}


extern dispatcher_handler_factory_fn root_factories[];


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

        IncomingContext context;
        FactoryDispatcherHandler fdh(dispatcherBuffer, context, root_factories, 2);
        Dispatcher dispatcher;
        pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
        embr::coap::experimental::BlockingEncoder encoder(writer);

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
            
#ifndef ESP32
            printf("\r\nResponding: %d to %d.%d.%d.%d:%d", len,
                ip4_addr1_16(from_ip),
                ip4_addr2_16(from_ip),
                ip4_addr3_16(from_ip),
                ip4_addr4_16(from_ip),
                from_port);
#endif

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
