extern "C" {

#include <lwip/api.h>
#include <lwip/udp.h>

}

#include "context.h"

void do_notify(AppContext& context, struct pbuf* p);

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    if (p != NULL)
    {
        AppContext context(pcb, addr, port);

        // NOTE: do_notify plumbing calls pbuf_free(p)
        do_notify(context, p);
    }
}



void udp_coap_init(void)
{
    struct udp_pcb * pcb;

    /* get new pcb */
    pcb = udp_new();
    if (pcb == NULL) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return;
    }

    /* bind to any IP address */
    if (udp_bind(pcb, IP_ADDR_ANY, COAP_UDP_PORT) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    udp_recv(pcb, udp_coap_recv, NULL);
}