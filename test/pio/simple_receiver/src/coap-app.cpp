#include <coap.h>
#include <coap-uripath-dispatcher.h>

#include "lwip/api.hpp"

#define COAP_UDP_PORT 5683

using namespace moducom;
using namespace moducom::coap;
using namespace moducom::coap::experimental;

// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;

constexpr char STR_URI_V1[] = "v1";
constexpr char STR_URI_TEST[] = "test";

class TestDispatcherHandler : public DispatcherHandlerBase
{
public:
    void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) override
    {
        printf("Got payload: %d", payload_part.length());
    }
};

extern dispatcher_handler_factory_fn v1_factories[];

dispatcher_handler_factory_fn root_factories[] =
{
    uri_plus_factory_dispatcher<STR_URI_V1, v1_factories, 1>
};

dispatcher_handler_factory_fn v1_factories[] = 
{
    uri_plus_observer_dispatcher<STR_URI_TEST, TestDispatcherHandler>
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

        printf("\r\nGot COAP data: %d", len);

        dispatcher.head(&fdh);
        dispatcher.dispatch(chunk);

        buf.free();
    }
}
