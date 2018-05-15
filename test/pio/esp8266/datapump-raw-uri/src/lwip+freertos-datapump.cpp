#include <exp/datapump.hpp>
#include <platform/lwip/lwip-datapump.h>
#include <lwip/api.hpp>

#include <lwipopts.h>

// FIX: temporarily conflating datapump and uri responder
#include <simple-uri-responder.hpp>

using namespace moducom::coap;

lwip_datapump_t datapump;

extern "C" void coap_daemon(void *)
{
    LwipDatapumpHelper ldh;

    printf("CoAP daemon running\n");

    for(;;)
    {
        ldh.loop(datapump);

        datapump.service(simple_uri_responder, true);
    }
}