#pragma once

#include <esp_log.h>

#include <estd/optional.h>

#include <embr/platform/esp-idf/ledc.h>

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

    struct states
    {
        struct undefined
        {

        };

        struct gpio
        {

        };

        struct ledc_timer
        {

        };

        struct pwm
        {
            // DEBT: Use embr::esp_idf::ledc here

            struct
            {
                ledc_channel_config_t channel;

            }   config;

            estd::layer1::optional<uint16_t, 0xFFFF> duty;

            pwm(int channel);
        };
    
    };

    // NOTE: This is likely a better job for variant_storage, since we know based on URI which particular
    // state we're interested in and additionally we'd prefer not to initialize *any* - so in other words
    // somewhere between a union and a variant, which is what variant_storage really is
    estd::variant<
        states::undefined,
        states::ledc_timer,
        states::gpio,
        states::pwm> state;

    enum states_enum
    {
        STATE_UNDEFINED,
        STATE_LEDC_TIMER,
        STATE_GPIO,
        STATE_PWM
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
