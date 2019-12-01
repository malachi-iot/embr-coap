#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/pbuf.h>

struct AppContext : moducom::coap::IncomingContext<ip_addr_t>
{
    struct udp_pcb* pcb;
    int port;

    typedef StreambufEncoder<out_pbuf_streambuf> encoder_type;

    AppContext(struct udp_pcb* _pcb, 
        const ip_addr_t* addr,
        int port) :
        pcb(_pcb),
        port(port)
    {
        this->addr = *addr;
    }

    encoder_type make_encoder()
    {
        return encoder_type(32);
    }
};