#pragma once

#include <coap/platform/lwip/context.h>

struct AppContext : 
    moducom::coap::IncomingContext<const ip_addr_t*>,
    moducom::coap::LwipContext
{
    uint16_t port;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb),
        port(port)
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
        sendto(encoder, this->address(), port);
    }


    template <class TSubject>
    void do_notify(pbuf_pointer p, TSubject& subject)
    {
        LwipContext::do_notify(subject, *this, p);
    }
};