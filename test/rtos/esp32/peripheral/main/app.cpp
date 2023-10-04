#include <estd/istream.h>

#include <embr/observer.h>

#include <embr/platform/esp-idf/board.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/rtc.h>

using namespace embr::coap;

#include "context.h"

#ifdef ESP_PLATFORM
// This gets us 'build_version_response' which is indeed esp-idf specific
#include <coap/platform/esp-idf/observer.h>
#endif

#include <coap/platform/lwip/encoder.h>

#include <coap/platform/api.hpp>


using istreambuf_type = AppContext::istreambuf_type;

enum
{
    id_path_v1 = 0,
    id_path_v1_api,
    id_path_v1_api_analog,
    id_path_v1_api_gpio,
    id_path_v1_api_time,
    id_path_v1_api_gpio_value,
    id_path_v1_api_pwm,
    id_path_v1_api_pwm_value,

    id_path_well_known,
    id_path_well_known_core
};


using embr::coap::internal::UriPathMap;


// NOTE: Alphabetization per path segment is important.  id# ordering is not
// DEBT: Document this behavior in detail
const UriPathMap uri_map[] =
{
    { "v1",         id_path_v1,                 MCCOAP_URIPATH_NONE },
    { "api",        id_path_v1_api,             id_path_v1 },
    { "analog",     id_path_v1_api_analog,      id_path_v1_api },
    { "gpio",       id_path_v1_api_gpio,        id_path_v1_api },
    { "*",          id_path_v1_api_gpio_value,  id_path_v1_api_gpio },
    { "ledc",       id_path_v1_api_pwm,         id_path_v1_api },
    { "*",          id_path_v1_api_pwm_value,   id_path_v1_api_pwm },
    { "time",       id_path_v1_api_time,        id_path_v1_api },

    EMBR_COAP_V1_SYS_PATHS(id_path_v1),

    { ".well-known",    id_path_well_known,         MCCOAP_URIPATH_NONE },
    { "core",           id_path_well_known_core,    id_path_well_known }
};


struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(event::option_start, AppContext& context)
    {
        context.uri_int.reset();
    }


    using query = estd::pair<estd::string_view, estd::string_view>;

    static query split(const event::option& e)
    {
        const estd::string_view sv = e.string();
        const estd::size_t split_pos = sv.find('=');

        if(split_pos == estd::string_view::npos)
            return { };

        const estd::string_view key(sv.substr(0, split_pos)),
            value(sv.substr(split_pos + 1));

        return { key, value };
    }


    // DEBT: This can work, but we'll need the internal from_chars which
    // takes iterators.  That likely comes with some kind of performance
    // penalty.  Related DEBT from string_view and friends itself which
    // duplicate data(), etc. 
    //template <class Impl, class Int>
    //static estd::from_chars_result from_string(
    //    const estd::detail::basic_string<Impl>& s, Int& v)
    // DEBT: Consider placing this directly into estd, seems useful
    // DEBT: Filter Int by numeric/int
    template <class Int>
    static estd::from_chars_result from_string(
        const estd::string_view& s, Int& v)
    {
        return estd::from_chars(s.begin(), s.end(), v);
    }

    template <class Int>
    static estd::from_chars_result from_query(
        const query& q, const char* key, Int& v)
    {
        const estd::string_view value = estd::get<1>(q);

        if(estd::get<0>(q).compare(key) == 0)
        {
            const estd::from_chars_result r = from_string(value, v);

            if(r.ec != 0)
            {
                ESP_LOGW(TAG, "from_query: key=%s cannot parse value");
            }
            else
            {
                long long debug_v = v;
                ESP_LOGD(TAG, "from_query: key=%s value=%ll", debug_v);
            }

            return r;
        }

        // DEBT: Use argument_out_of_domain instead, since
        // result_out_of_range is a from_string-generated error
        return { nullptr, estd::errc::result_out_of_range };
        //return { nullptr, estd::errc::argument_out_of_domain };
    }

    static query on_uri_query(const event::option& e, AppContext& context)
    {
        const query q = split(e);
        const estd::string_view key = estd::get<0>(q);
        const estd::string_view value = estd::get<1>(q);

        unsigned v;

        switch(context.state.index())
        {
            case AppContext::STATE_LEDC_TIMER:
            {
                auto& s = estd::get<AppContext::states::ledc_timer>(context.state);

                // Bring out freq_hz & duty_resolution
                // Later consider bringing out timer_num
                if(key.compare("freq_hz") == 0)
                {
                    // DEBT: Pay attention to return value
                    from_string(value, v);

                    ESP_LOGD(TAG, "on_uri_query: freq_hz=%u", v);

                    s.config.freq_hz = v;
                }
                else if(key.compare("duty_resolution"))
                {
                    // DEBT: Pay attention to return value
                    from_string(value, v);

                    ESP_LOGD(TAG, "on_uri_query: duty_resolution=%u", v);

                    // DEBT: esp-idf the enum matches up, but I don't think
                    // that's promised anywhere
                    s.config.duty_resolution = (ledc_timer_bit_t)v;
                }
                break;
            }

            case AppContext::STATE_LEDC_CHANNEL:
            {
                auto& s = estd::get<AppContext::states::ledc_channel>(context.state);

                // Bring out duty, channel, gpio
                // If only duty, don't do channel init
                if(key.compare("duty") == 0)
                {
                    // DEBT: Pay attention to return value
                    from_string(value, v);

                    ESP_LOGD(TAG, "on_uri_query: duty=%u", v);

                    s.config.duty = v;
                }
                else if(key.compare("channel") == 0)
                {
                    // DEBT: Pay attention to return value
                    from_string(value, v);

                    ESP_LOGD(TAG, "on_uri_query: channel=%u", v);

                    s.has_config = true;
                    s.config.channel = (ledc_channel_t) v;
                }
                else if(key.compare("pin") == 0)
                {
                    // DEBT: Pay attention to return value
                    from_string(value, v);

                    ESP_LOGD(TAG, "on_uri_query: pin=%u", v);

                    s.has_config = true;
                    s.config.gpio_num = v;
                }
                break;
            }

            default:
                break;
        }

        return q;
    } 


    static void on_notify(const event::option& e, AppContext& context)
    {
        if(e.option_number == Option::UriQuery)
        {
            on_uri_query(e, context);
            return;
        }

        if(e.option_number != Option::UriPath) return;

        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
                context.select_gpio(e);
                break;

            case id_path_v1_api_pwm:
                context.state.emplace<AppContext::states::ledc_timer>();
                break;

            case id_path_v1_api_pwm_value:
                context.state.emplace<AppContext::states::ledc_channel>();
                context.select_pwm_channel(e);
                break;
        }
    }

    static void on_notify(event::streambuf_payload<istreambuf_type> e, AppContext& context)
    {
        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
                context.put_gpio(e.streambuf);
                break;

            case id_path_v1_api_pwm:
                // DEBT: Eventually gather timer # here
                break;

            case id_path_v1_api_pwm_value:
                context.put_pwm(e.streambuf);
                break;
        }
    }

    
    __attribute__ ((noinline))  // Not necessary, just useful for stack usage analysis
    static void on_notify(event::completed, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        auto encoder = make_encoder(context);

        switch(context.found_node())
        {
            case id_path_v1_api_gpio:
                context.response_code = Header::Code::NotImplemented;
                break;

            case id_path_v1_api_analog:
                context.completed_analog(encoder);
                break;
                
            case id_path_v1_api_gpio_value:
                context.completed_gpio(encoder);
                break;

            case id_path_v1_api_pwm:
                context.response_code = Header::Code::NotImplemented;
                break;

            case id_path_v1_api_pwm_value:
                context.completed_pwm(encoder);
                break;

            case id_path_v1_api_time:
                send_time_response(context, encoder);
                break;
                
            default:
                sys_paths::send_sys_reply(context, encoder);
                break;
        }

        auto_reply(context, encoder);
    }
};


embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver,
    UriParserObserver,
    Observer
    > app_subject;


AppContext::AppContext(struct udp_pcb* pcb, 
    const ip_addr_t* addr,
    uint16_t port) : 
    LwipIncomingContext(pcb, addr, port),
    UriParserContext(uri_map)
{

}


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    // Because AppContext is on the stack and app_subject is layer0 (stateless)
    // This code is, in theory, reentrant.  That has not been tested well, however
    AppContext context(pcb, addr, port);

    ESP_LOGD(TAG, "p->len=%d, sizeof context=%u", p->len, sizeof(context));

    // _recv plumbing depends on us to free p, and decode_and_notify here helps us with that
    // DEBT: I think I prefer explicitly freeing here, decode_and_notify assistance is too magic
    decode_and_notify(p, app_subject, context);
}


void initialize_adc();
void initialize_ledc();

void app_init(void** arg)
{
#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
    const char* TAG = "app_init";

    const esp_reset_reason_t reset_reason = esp_reset_reason();

    ESP_LOGD(TAG, "reset_reason=%d", reset_reason);

    // As per
    // https://stackoverflow.com/questions/69880289/c-int-undefined-behaviour-when-stored-as-rtc-noinit-attr-esp32
    switch(reset_reason)
    {
        case ESP_RST_UNKNOWN:       // ESP32C3 starts after flash with this reason
        case ESP_RST_EXT:
        case ESP_RST_POWERON:
            esp_idf::reboot_counter = 0;
            esp_idf::panic_reboot_counter = 0;
            ESP_LOGI(TAG, "Poweron");
            break;

        case ESP_RST_PANIC:
            ++esp_idf::panic_reboot_counter;
            [[fallthrough]];

        default:
            ++esp_idf::reboot_counter;
            break;
    }
#endif

    initialize_adc();
    initialize_ledc();
    initialize_sntp();
    initialize_mdns();
}


#if FEATURE_APP_GPIO == 0
void AppContext::select_gpio(const event::option& e) {}
void AppContext::put_gpio(istreambuf_type& streambuf) {}
void AppContext::completed_gpio(encoder_type& encoder)
{
    build_reply(*this, encoder, Header::Code::NotImplemented);
    reply(encoder);
}
#endif