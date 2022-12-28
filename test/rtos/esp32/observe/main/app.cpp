#include "esp_log.h"

#include <coap/platform/lwip/encoder.h>

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    
}