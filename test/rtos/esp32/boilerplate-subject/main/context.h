#pragma once

#include "lwip-context.h"
#include "observer-util.h"

typedef moducom::coap::experimental::UriPathMap UriPathMap;

extern const UriPathMap uri_map[7];

struct AppContext : 
    moducom::coap::IncomingContext<const ip_addr_t*>,
    LwipContext,
    UriParserContext
{
    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb, port),
        UriParserContext(uri_map)
    {
        this->addr = addr;
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


inline AppContext::encoder_type make_encoder(const AppContext&)
{
    ESP_LOGD("TEST", "Got to special encoder");

    return AppContext::encoder_type(32);
}