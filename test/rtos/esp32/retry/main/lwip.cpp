#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/udp.h>

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

using namespace embr;

template <bool use_ptr>
err_t send_rom(lwip::udp::Pcb pcb, struct pbuf* pbuf,
    const lwip::internal::Endpoint<use_ptr>& endpoint)
{
    lwip::v1::Pbuf pbuf_rom(0, PBUF_TRANSPORT, PBUF_ROM);

    if(pbuf_rom.valid() == false)   return ERR_BUF;

    pbuf_rom.chain(pbuf);

    return pcb.send_experimental(pbuf_rom, endpoint);
}



void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port);

void app_init(void** pcb_recv_arg, embr::lwip::udp::Pcb pcb);

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
