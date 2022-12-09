#pragma once

#include <coap/platform/lwip/context.h>
// TODO: We mainly include util.h for UriParserContext.  Include more obvious/cleaner include source once
// the URI code base is cleaned up
#include <coap/decoder/observer/util.h>

typedef embr::coap::internal::UriPathMap UriPathMap;

// NOTE: Not ideal, directly specifying '8' here
extern const UriPathMap uri_map[8];

struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext
{
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipIncomingContext(pcb, addr, port),
        UriParserContext(uri_map)
    {
    }

    union
    {
        struct
        {
            int pin;

        }   gpio;
    };

    void select_gpio(const embr::coap::event::option& e);
    void put_gpio(istreambuf_type& payload);
    void get_gpio(encoder_type& encoder);
};


inline AppContext::encoder_type make_encoder(const AppContext&)
{
    ESP_LOGV("TEST", "Got to special encoder");

    return AppContext::encoder_type(32);
}