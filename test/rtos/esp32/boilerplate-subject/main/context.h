#pragma once

#include <coap/platform/lwip/context.h>
// TODO: We mainly include util.h for UriParserContext.  Include more obvious/cleaner include source once
// the URI code base is cleaned up
#include <coap/decoder/observer/util.h>

typedef moducom::coap::experimental::UriPathMap UriPathMap;

// NOTE: Not ideal, directly specifying '7' here
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
};


inline AppContext::encoder_type make_encoder(const AppContext&)
{
    ESP_LOGD("TEST", "Got to special encoder");

    return AppContext::encoder_type(32);
}