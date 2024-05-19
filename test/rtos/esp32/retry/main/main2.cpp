/**
 * @file
 * @author Malachi Burke
 * esp-idf LwIP UDP specific initialization.  No TCP or DTLS support
 */
#include <esp-helper.h>

#include <embr/platform/lwip/udp.h>

#include "app.h"

#ifndef FEATURE_IDF_DEFAULT_EVENT_LOOP
#error
#endif

void udp_coap_init(void* pcb_recv_arg);

esp_netif_t* wifi_netif;


extern "C" void app_main()
{
    init_flash();
    
    wifi_netif = wifi_init_sta();

    void* pcb_recv_arg = nullptr;

    // Doesn't coexist at the moment
    //app::esp_now::init();
    udp_coap_init(pcb_recv_arg);

    app::loop();
}

