#pragma once

#include "esp_log.h"

#include <estd/optional.h>

#include <coap/platform/lwip/context.h>

// TODO: We mainly include util.h for UriParserContext.  Include more obvious/cleaner include source once
// the URI code base is cleaned up
#include <coap/decoder/observer/util.h>

#include "features.h"

struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext
{
    static constexpr const char* TAG = "AppContext";

    typedef embr::lwip::ipbuf_streambuf istreambuf_type;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port);

    // Gathered from option URI
    union
    {
        struct
        {
            estd::layer1::optional<int16_t, -1> pin;

        }   gpio;

        // (last) integer which appears on URI list
        estd::layer1::optional<int16_t, -1> uri_int;
    };

    estd::layer1::optional<uint16_t, 0xFFFF> pwm_value;

    void select_gpio(const embr::coap::event::option& e);
    void select_pwm_channel(const embr::coap::event::option&);
    void put_gpio(istreambuf_type& payload);
    void put_pwm(istreambuf_type& payload);
    void completed_gpio(encoder_type&);

    void completed_analog(encoder_type&);
    void completed_pwm(encoder_type&);
};


void initialize_sntp();
void initialize_mdns();
void send_time_response(AppContext& context, AppContext::encoder_type& encoder);
