#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

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
