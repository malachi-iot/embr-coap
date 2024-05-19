/**
 * @file
 * @author Malachi Burke
 * esp-idf LwIP UDP specific initialization.  No TCP or DTLS support
 */
#include <esp-helper.h>

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

#ifndef FEATURE_IDF_DEFAULT_EVENT_LOOP
#error
#endif

void udp_coap_init(void* pcb_recv_arg);

esp_netif_t* wifi_netif;

void espnow_wifi_init(void);

void app_loop();


extern "C" void app_main()
{
    init_flash();
    
    wifi_netif = wifi_init_sta();

    void* pcb_recv_arg = nullptr;

    udp_coap_init(pcb_recv_arg);

    app_loop();
}

