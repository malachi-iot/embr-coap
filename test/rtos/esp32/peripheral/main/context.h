#pragma once

#include <concepts>

#include <esp_log.h>

#include <estd/optional.h>
#include <estd/string_view.h>

#include <embr/platform/esp-idf/ledc.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/esp-idf/subcontext/gpio.h>
#include <coap/platform/esp-idf/subcontext/ledc.h>

// TODO: We mainly include util.h for UriParserContext.  Include more obvious/cleaner include source once
// the URI code base is cleaned up
#include <coap/decoder/observer/util.h>

#include "features.h"
#include "paths.h"
#include "subcontext.h"

extern const embr::coap::internal::UriPathMap uri_map[];



struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext
{
    static constexpr const char* TAG = "AppContext";

    using istream_type = estd::detail::basic_istream<istreambuf_type&>;
    //using query = estd::pair<estd::string_view, estd::string_view>;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port);

    embr::coap::experimental::Pipeline<
        embr::coap::experimental::UriValuePipeline<int, -1> > pipeline;

    struct states
    {
        using base = embr::coap::internal::v1::SubcontextBase::base<AppContext>;

        struct analog : base
        {
            static constexpr int id_path = id_path_v1_api_analog;

            constexpr analog(AppContext& c) : base{c} {}

            code_type response() const;
            bool completed(encoder_type&);
        };

        using gpio = embr::coap::esp_idf::gpio<AppContext, id_path_v1_api_gpio_value>;

        struct ledc_timer : base
        {
            static constexpr const char* TAG = "states::ledc_timer";

            static constexpr int id_path = id_path_v1_api_pwm;

            ledc_timer_config_t config;

            ledc_timer(AppContext&);
            ledc_timer(AppContext& c, const ledc_timer_config_t&) : ledc_timer(c) {}

            void on_option(const query&);
            code_type response() const;
        };


        using ledc_timer2 = embr::coap::esp_idf::subcontext::ledc_timer<AppContext, id_path_v1_api_pwm>;
        using ledc_channel = embr::coap::esp_idf::subcontext::ledc_channel<AppContext, id_path_v1_api_pwm_value>;
    };

    embr::coap::Subcontext<
        states::analog,
        states::ledc_timer,
        states::gpio,
        states::ledc_channel> state;

    // NOTE: These only coincidentally conform to subject/observer name convention

    bool on_notify(const embr::coap::event::option&);
};


void initialize_sntp();
void initialize_mdns();
void send_time_response(AppContext& context, AppContext::encoder_type& encoder);
embr::coap::Header::Code get_coap_code(esp_err_t);

