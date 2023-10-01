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
    { "time",       id_path_v1_api_time,        id_path_v1_api },

    EMBR_COAP_V1_SYS_PATHS(id_path_v1),

    { ".well-known",    id_path_well_known,         MCCOAP_URIPATH_NONE },
    { "core",           id_path_well_known_core,    id_path_well_known }
};


struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(const event::option& e, AppContext& context)
    {
        if(context.found_node() == id_path_v1_api_gpio_value)
            context.select_gpio(e);
    }

    static void on_notify(event::streambuf_payload<istreambuf_type> e, AppContext& context)
    {
        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
                context.put_gpio(e.streambuf);
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