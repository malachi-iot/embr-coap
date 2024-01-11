#include <estd/istream.h>

#include <embr/observer.h>

#include <embr/platform/esp-idf/board.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/rtc.h>

using namespace embr::coap;

#define WIFI_POLL_TIMEOUT_S 30

#include "context.h"
#include "ip.h"

#ifdef ESP_PLATFORM
// This gets us 'build_version_response' which is indeed esp-idf specific
#include <coap/platform/esp-idf/observer.h>
#endif

#include <coap/platform/lwip/encoder.h>

#include <coap/platform/api.hpp>
#include <coap/platform/esp-idf/subcontext/gpio.hpp>
#include <coap/platform/esp-idf/subcontext/ledc.hpp>


using istreambuf_type = AppContext::istreambuf_type;

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
    EMBR_COAP_CoRE_PATHS()
};


struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(event::option_start, AppContext& context)
    {
    }


    static void on_notify(const event::option& e, AppContext& context)
    {
        context.on_notify(e);
    }

    static void on_notify(event::streambuf_payload<istreambuf_type> e, AppContext& context)
    {
        context.state.on_payload(e.streambuf);
    }

    
    __attribute__ ((noinline))  // Not necessary, just useful for stack usage analysis
    static void on_notify(event::completed e, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        auto encoder = make_encoder(context);

        switch(context.found_node())
        {
            case id_path_v1_api_gpio:
                context.response_code = Header::Code::NotImplemented;
                break;

            case id_path_v1_api_time:
                send_time_response(context, encoder);
                break;
                
            default:
                if(!context.state.on_completed(encoder, context))
                    sys_paths::send_sys_reply(context, encoder);

                break;
        }

        auto_reply(context, encoder);
    }
};



// DEBT: It seems that we can fuse layer0 and layer1 together now that sparse
// tuple is underneath
embr::layer1::subject<
    embr::coap::experimental::PipelineObserver,
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

    ESP_LOGV(TAG, "p->len=%d, sizeof context=%u", p->len, sizeof(context));

    // _recv plumbing depends on us to free p, and decode_and_notify here helps us with that
    // DEBT: I think I prefer explicitly freeing here, decode_and_notify assistance is too magic
    decode_and_notify(p, app_subject, context);
}


void initialize_adc();
void initialize_ledc();
void initialize_ip_retry();

void app_init(void** arg)
{
    const char* TAG = "app_init";

    ESP_LOGD(TAG, "sizeof(app_subject)=%d", sizeof(app_subject));

#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
    const esp_reset_reason_t reset_reason = esp_idf::track_reset_reason();
#else
    const esp_reset_reason_t reset_reason = esp_reset_reason();
#endif

    ESP_LOGD(TAG, "reset_reason=%d", reset_reason);

    initialize_adc();
    initialize_ledc();
    initialize_ip_retry();
    initialize_sntp();
    initialize_mdns();
}

void app_loop()
{
    static constexpr const char* TAG = "app_loop";

    ESP_LOGI(TAG, "started");

    for(;;)
    {
#if !FEATURE_WIFI_POLL_USE_RTOS_TIMER        
        wifi_retry_poll();
#endif
        estd::this_thread::sleep_for(estd::chrono::seconds(60));
    }
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
