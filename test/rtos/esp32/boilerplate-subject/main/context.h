#pragma once

#include <coap/platform/lwip/context.h>
// TODO: We mainly include util.h for UriParserContext.  Include more obvious/cleaner include source once
// the URI code base is cleaned up
#include <coap/decoder/observer/util.h>

struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext
{
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port);

    union
    {
        struct
        {
            int pin : 16;

        }   gpio;
    };

    void select_gpio(const embr::coap::event::option& e);
    void put_gpio(istreambuf_type& payload);
    void completed_gpio(encoder_type& encoder);
};


// DEBT: non-obvious LwIP specificity
inline AppContext::encoder_type make_encoder(const AppContext&)
{
    ESP_LOGV("TEST", "Got to special encoder");

    return AppContext::encoder_type(32);
}