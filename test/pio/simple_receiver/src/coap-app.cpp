#include "coap.h"

#include "lwip/api.hpp"

#define COAP_UDP_PORT 5683

using namespace moducom;
using namespace moducom::coap;
using namespace moducom::coap::experimental;

// in-place new holder
static pipeline::layer3::MemoryChunk<256> dispatcherBuffer;

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

        pipeline::MemoryChunk((uint8_t*)data, len);

        //FactoryDispatcherHandler fdh(dispatcherBuffer, test_factories, 3);

        printf("\r\nGot COAP data: %d", len);

        buf.free();
    }
}
