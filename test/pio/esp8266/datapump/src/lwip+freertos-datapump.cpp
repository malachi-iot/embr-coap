#include <exp/datapump.hpp>
#include <platform/lwip/lwip-datapump.h>
#include <lwip/api.hpp>

// FIX: temporarily conflating datapump and ping responder
#include <simple-ping-responder.hpp>

using namespace moducom::coap;

lwip_datapump_t datapump;

extern "C" void coap_daemon(void *)
{
    LwipDatapumpHelper ldh;

    for(;;)
    {
        ldh.loop(datapump);

        simple_ping_responder(ldh, datapump);
    }
}