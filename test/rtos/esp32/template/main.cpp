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

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

// Very early app init, even before nvs initialization
__attribute__((weak))
void app_init() {}

// Obsolete, use above or below version
__attribute__((weak))
void app_init(void** pcb_recv_arg) {}

__attribute__((weak))
void app_init(void** pcb_recv_arg, embr::lwip::udp::Pcb pcb) {}

__attribute__((weak))
void app_loop() {}

void udp_coap_init(void* pcb_recv_arg)
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

    app_init(&pcb_recv_arg, pcb);

    /* set udp_echo_recv() as callback function
       for received packets */
    pcb.recv(udp_coap_recv, pcb_recv_arg);
}

esp_netif_t* wifi_netif;

extern "C" void app_main()
{
    app_init();

    init_flash();
    
    wifi_netif = wifi_init_sta();

    void* pcb_recv_arg = nullptr;

    app_init(&pcb_recv_arg);

    udp_coap_init(pcb_recv_arg);

    app_loop();
}

