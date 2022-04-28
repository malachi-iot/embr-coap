#pragma once

#include <coap/platform/lwip/context.h>

#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
typedef  embr::coap::LwipIncomingContext AppContext;
#else
struct AppContext : 
    embr::coap::IncomingContext<const ip_addr_t*>,
    embr::coap::LwipContext
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

    void reply(encoder_type& encoder)
    {
        sendto(encoder, this->address(), port);
    }


    // convenience method
    template <class TSubject>
    void do_notify(pbuf_pointer p, TSubject& subject)
    {
        LwipContext::do_notify(subject, *this, p);
    }
};
#endif

// number of seed bytes is sorta app-specific, so do it here
// (ping needs only 8 bytes ever)
inline LwipContext::encoder_type make_encoder(const AppContext&)
{
    return LwipContext::encoder_type(8);
}
