#pragma once

#include <coap/platform/lwip/context.h>
#include <coap/decoder/subject.h>

typedef moducom::coap::experimental::UriPathMap UriPathMap;

extern const UriPathMap uri_map[7];

struct AppContext : 
    moducom::coap::LwipIncomingContext,
    moducom::coap::UriParserContext
{
    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipIncomingContext(pcb, addr, port),
        UriParserContext(uri_map)
    {
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