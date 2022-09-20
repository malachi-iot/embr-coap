/**
 * @file
 * @author Malachi Burke
 * esp-idf LwIP UDP specific initialization.  No TCP or DTLS support
 */
#include <esp-helper.h>

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

__attribute__((weak))
void app_init(void** pcb_recv_arg) {}

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

    /* set udp_echo_recv() as callback function
       for received packets */
    pcb.recv(udp_coap_recv, pcb_recv_arg);
}



extern "C" void app_main()
{
    init_flash();
    
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
    wifi_init_sta();
#else
    wifi_init_sta(event_handler);
#endif

    void* pcb_recv_arg = nullptr;

    app_init(&pcb_recv_arg);

    udp_coap_init(pcb_recv_arg);

    app_loop();
}

