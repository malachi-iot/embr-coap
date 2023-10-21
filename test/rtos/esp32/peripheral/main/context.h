#pragma once

#include <concepts>

#include <esp_log.h>

#include <estd/optional.h>
#include <estd/string_view.h>

#include <embr/platform/esp-idf/ledc.h>

#include <coap/platform/lwip/context.h>

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

    typedef embr::lwip::ipbuf_streambuf istreambuf_type;
    using istream_type = estd::detail::basic_istream<istreambuf_type&>;
    using query = estd::pair<estd::string_view, estd::string_view>;

    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port);

    // (last) integer which appears on URI list
    estd::layer1::optional<int16_t, -1> uri_int;

    void populate_uri_int(const embr::coap::event::option&);

    struct states
    {
        using code_type = embr::coap::Header::Code;

        struct undefined
        {
            static bool constexpr on_option(const query&) { return {}; }
            static bool constexpr on_payload(istream_type&) { return {}; }
            static constexpr code_type response()
            {
                return code_type::Empty;
            }
            static constexpr bool completed(encoder_type&)
            {
                return {};
            }
        };

        struct unknown : undefined
        {
            static constexpr int id_path = -1;

            unknown() = default;
            constexpr unknown(AppContext&) {}   // dummy, just for factory to be satisfied
        };

        struct base : undefined
        {
            AppContext& context;

            const embr::coap::Header& header() const
            {
                return context.header();
            }

            /*
            void build_reply(encoder_type& e, code_type c)
            {
                embr::coap::build_reply(context, encoder, code);
            }   */

            constexpr base(AppContext& c) : context{c} {}
        };

        struct analog : base
        {
            static constexpr int id_path = id_path_v1_api_analog;

            constexpr analog(AppContext& c) : base{c} {}

            code_type response() const;
            bool completed(encoder_type&);
        };

        struct gpio : base
        {
            static constexpr const char* TAG = "states::gpio";

            static constexpr int id_path = id_path_v1_api_gpio_value;

            constexpr gpio(AppContext& c) : base(c) {}

            estd::layer1::optional<bool> level;

            void on_payload(istream_type&);
            code_type response() const;
            bool completed(encoder_type&);
        };

        struct ledc_timer : base
        {
            static constexpr const char* TAG = "states::ledc_timer";

            static constexpr int id_path = id_path_v1_api_pwm;

            ledc_timer_config_t config;

            ledc_timer(AppContext&);

            void on_option(const query&);
            code_type response() const;
        };

        struct ledc_channel : base
        {
            static constexpr const char* TAG = "states::ledc_channel";

            static constexpr int id_path = id_path_v1_api_pwm_value;

            // DEBT: Use embr::esp_idf::ledc here

            ledc_channel_config_t config;

            // We always use above config, but if this is true it signals a call
            // to channel configure itself vs a mere duty update
            bool has_config = false;

            estd::layer1::optional<uint16_t, 0xFFFF> duty;

            ledc_channel(AppContext&);

            void on_option(const query&);
            void on_payload(istream_type&);
            code_type response();
        };
    };

    // NOTE: This is likely a better job for variant_storage, since we know based on URI which particular
    // state we're interested in and additionally we'd prefer not to initialize *any* - so in other words
    // somewhere between a union and a variant, which is what variant_storage really is
    CoapSubcontext<
        states::unknown,
        states::analog,
        states::ledc_timer,
        states::gpio,
        states::ledc_channel> state;

    // NOTE: These only coincidentally conform to subject/observer name convention

    void on_payload(istreambuf_type&);
    bool on_notify(const embr::coap::event::option&);
    bool on_completed(encoder_type&);
};


void initialize_sntp();
void initialize_mdns();
void send_time_response(AppContext& context, AppContext::encoder_type& encoder);
embr::coap::Header::Code get_coap_code(esp_err_t);

