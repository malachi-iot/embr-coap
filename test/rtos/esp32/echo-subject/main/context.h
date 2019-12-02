#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/pbuf.h>

struct LwipContext
{
    typedef struct udp_pcb* pcb_pointer;
    typedef StreambufEncoder<out_pbuf_streambuf> encoder_type;

    pcb_pointer pcb;
    uint16_t port;

    LwipContext(pcb_pointer pcb,
        uint16_t port) : 
        pcb(pcb),
        port(port) {}
};

struct AppContext : 
    moducom::coap::IncomingContext<const ip_addr_t*>,
    LwipContext
{
    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb, port)
    {
        this->addr = addr;
    }

    // number of seed bytes is sorta app-specific, so do it here
    // (ping needs only 8 bytes ever)
    encoder_type make_encoder()
    {
        return encoder_type(8);
    }

    void reply(encoder_type& encoder)
    {
        udp_sendto(pcb, 
            encoder.rdbuf()->netbuf().pbuf(), 
            this->address(), 
            port);
    }
};