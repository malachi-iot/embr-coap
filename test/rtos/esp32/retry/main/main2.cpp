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
    
#if CONFIG_UDP
    wifi_netif = wifi_init_sta();

    void* pcb_recv_arg = nullptr;
#endif

    // Doesn't coexist at the moment
#if CONFIG_ESP_NOW
    app::esp_now::init();
#endif
#if CONFIG_UDP
    udp_coap_init(pcb_recv_arg);
#endif

    app::loop();
}

