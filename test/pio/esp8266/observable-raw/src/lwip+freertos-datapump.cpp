#include <exp/datapump.hpp>
#include <platform/lwip/lwip-datapump.h>
#include <lwip/api.hpp>

#include <FreeRTOS/task.h>

// newer versions of FreeRTOS use portTICK_PERIOD_MS
#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_RATE_MS
#endif

#include <lwipopts.h>

// FIX: temporarily conflating datapump and observable-raw test
#include <observable-raw-responder.hpp>

using namespace embr::coap;

lwip_datapump_t datapump;

layer1::ObservableRegistrar<lwip_datapump_t::addr_t, 10> observable_registrar;

extern "C" void coap_daemon(void *)
{
    LwipDatapumpHelper ldh;

    printf("CoAP daemon running\n");

    // unsurprisingly this didn't work, though interestingly it failed on 
    // the link phase
    //auto start = std::chrono::system_clock::now();

    portTickType last = xTaskGetTickCount();

    for(;;)
    {
        ldh.loop(datapump);

        portTickType current = xTaskGetTickCount();

        // if 1 second passes
        if(((xTaskGetTickCount() - last) / portTICK_PERIOD_MS) > 1000)
        {
            last = current;

            // iterate over all the registered observers and emit a coap message to them
            // whose contents are determined by emit_observe
            observable_registrar.for_each(datapump, emit_observe);
        }

    }
}