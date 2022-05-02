/**
 * @file
 * @author Malachi Burke
 * esp-idf LwIP UDP specific initialization.  No TCP or DTLS support
 */
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <esp-helper.h>
#include "esp_log.h"

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

void udp_coap_recv_freertos();

static StaticEventGroup_t xEventGroupBuffer;
EventGroupHandle_t xEventGroupHandle;


void udp_coap_init(void)
{
    embr::lwip::udp::Pcb pcb;

    // get new pcb
    if (!pcb.alloc()) {
        LWIP_DEBUGF(UDP_DEBUG, ("pcb.alloc failed!\n"));
        return;
    }

    /* bind to any IP address */
    if (pcb.bind(embr::coap::IP_PORT) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("pcb.bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    pcb.recv(udp_coap_recv);
}



extern "C" void app_main()
{
    const char *TAG = "app_main";

    init_flash();
    
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif

    xEventGroupHandle = xEventGroupCreateStatic(&xEventGroupBuffer);
    configASSERT(xEventGroupHandle);

    udp_coap_init();

    for(;;)
    {
        static unsigned counter = 0;

        ESP_LOGD(TAG, "counter=%u", ++counter);

        EventBits_t uxBits =
            xEventGroupWaitBits(xEventGroupHandle, 1, 1, 1, 10000 / portTICK_PERIOD_MS);

        if(uxBits == 1)
        {
            ESP_LOGW(TAG, "Got here");
            udp_coap_recv_freertos();
        }
    }
}

