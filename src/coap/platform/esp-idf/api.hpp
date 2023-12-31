#include <esp_wifi.h>

#include <estd/port/freertos/timer.h>

#include <estd/chrono.h>
#include <estd/thread.h>
//#include <estd/istream.h>

#include <embr/observer.h>
#include <embr/version.h>

#include <coap/context.h>
// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>
#include <coap/version.h>

#include "../../decoder/observer/core.h"
#include "../api.h"
#include "rtc.h"

#include <embr/json/encoder.hpp>

// DEBT: Really this all belongs under esp_idf namespace

namespace embr { namespace coap {

namespace internal {

inline void pended_restart(TimerHandle_t)
{
    static constexpr const char* TAG = "pended_restart";

    ESP_LOGI(TAG, "Restarting by request");

    esp_restart();
}

}

namespace sys_paths {

// TRegistrar refers to rfc7641 registration, not yet used
template <class TContext, class TRegistrar = estd::monostate, class enabled = void>
struct builder;

template <class TContext>
struct builder<TContext, estd::monostate,
    typename estd::enable_if<
        estd::is_base_of<tags::incoming_context, TContext>::value &&
        estd::is_base_of<UriParserContext, TContext>::value &&
        estd::is_base_of<internal::ExtraContext, TContext>::value>
        ::type
    >
{
    typedef TContext context_type;
    typedef typename context_type::encoder_type encoder_type;

    context_type& context;
    encoder_type& encoder;

    ESTD_CPP_CONSTEXPR_RET builder(context_type& context, encoder_type& encoder) :
        context(context), encoder(encoder)
    {}

    void auto_reply()
    {
        embr::coap::auto_reply(context, encoder);
    }

    void build_reply(Header::Code code)
    {
        embr::coap::build_reply(context, encoder, code);
    }

    void prep_payload(Header::Code code = Header::Code::Content)
    {
        build_reply(code);

        encoder.option(Option::ContentFormat, Option::ApplicationJson);
        encoder.payload();
    }

    void stats()
    {
        auto now = estd::chrono::freertos_clock::now();
        auto now_in_s = estd::chrono::seconds(now.time_since_epoch());

        wifi_ap_record_t wifidata;  // approximately 80 bytes of stack space
        esp_wifi_sta_get_ap_info(&wifidata);

        build_reply(Header::Code::Content);

        encoder.option(Option::ContentFormat, Option::ApplicationJson);

        encoder.payload();

        auto out = encoder.ostream();

        embr::json::v1::encoder json;
        auto j = make_fluent(json, out);

        j.begin()

        ("uptime", now_in_s.count())
        ("rssi", wifidata.rssi)
        ("free", esp_get_free_heap_size());

        j.end();
    }

    void mem()
    {
        build_reply(Header::Code::Content);

        encoder.option(Option::ContentFormat, Option::ApplicationJson);
        encoder.payload();

        auto out = encoder.ostream();

        embr::json::v1::encoder json;
        auto j = make_fluent(json, out);

        j.begin()

        ("free")
            ("heap", esp_get_free_heap_size())
            ("internal", esp_get_free_internal_heap_size())
        --
        ("stat")
            ("heap")
                ("min", esp_get_minimum_free_heap_size())
            --
        --

        .end();
    }

    void firmware_version_info()
    {
        prep_payload();

        auto out = encoder.ostream();

        embr::json::v1::encoder json;
        auto j = make_fluent(json, out);

    #if ESP_IDF_VERSION  >= ESP_IDF_VERSION_VAL(5, 0, 0)
        const esp_app_desc_t* app_desc = esp_app_get_description();
    #else
        const esp_app_desc_t* app_desc = esp_ota_get_app_description();
    #endif

        j.begin()

        ("s", "1.0.0")                // schema version
        ("app", app_desc->version)
        ("embr", EMBR_VERSION_STR)
        ("embr::coap", EMBR_COAP_VER_STR)
        ("idf", app_desc->idf_ver)

        .end();
    }

    void firmware_info()
    {
        prep_payload();
        auto out = encoder.ostream();

    #if ESP_IDF_VERSION  >= ESP_IDF_VERSION_VAL(5, 0, 0)
        const esp_app_desc_t* app_desc = esp_app_get_description();
    #else
        const esp_app_desc_t* app_desc = esp_ota_get_app_description();
    #endif

        embr::json::v1::encoder json;
        auto j = make_fluent(json, out);

        j.begin()

        ("name", app_desc->project_name)
        ("time", app_desc->time)
        ("date", app_desc->date)
        ("ver", app_desc->version)

        .end();
    }

#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
    void reboot_info()
    {
        prep_payload();
        auto out = encoder.ostream();

        embr::json::v1::encoder json;
        auto j = make_fluent(json, out);

        j.begin()

        ("wdt", embr::coap::esp_idf::wdt_reboot_counter)
        ("user", embr::coap::esp_idf::user_reboot_counter)
        ("brownout", embr::coap::esp_idf::brownout_reboot_counter)
        ("panic", embr::coap::esp_idf::panic_reboot_counter)
        ("other", embr::coap::esp_idf::other_reboot_counter)

        .end();
    }
#endif

    bool build_sys_reply()
    {
        switch(context.found_node())
        {
            case v1::root:
            {
                if(!verify(context, Header::Code::Get)) return false;
                
                stats();
                break;
            }

            case v1::root_memory:
                if(!verify(context, Header::Code::Get)) return false;
                
                mem();
                break;

            //case v1::root_uptime:
                //break;

            case v1::root_reboot:
            {
                //verified = verify(context, Header::Code::Put);

                //auto_reply(context, encoder);

                //if(!verified) return false;

                const Header::Code code = context.header().code();

                if(code == Header::Code::Get)
                {
#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
                    reboot_info();
#else
                    build_reply(Header::Code::ServiceUnavailable);
#endif
                }
                else if(verify(context, Header::Code::Put))
                {
                    TimerHandle_t h = xTimerCreate("restart delay",
                        pdMS_TO_TICKS(2000),
                        false,      // auto reload
                        nullptr,    // id
                        internal::pended_restart);

                    if(h != nullptr)
                    {
                        BaseType_t r = xTimerStart(h, pdMS_TO_TICKS(2000));

                        if(r == pdPASS)
                        {
                            build_reply(Header::Code::Changed);
                            return true;
                        }
                    }

                    build_reply(Header::Code::InternalServerError);
                }

                break;
            }

            case v1::root_firmware:
                if(!verify(context, Header::Code::Get)) return false;

                firmware_info();
                break;

            case v1::root_firmware_version:
                if(!verify(context, Header::Code::Get)) return false;

                firmware_version_info();
                break;

            // No send is requested
            default: return false;
        }

        // Indicate we built an encoder and send something
        return true;
    }
};



template <class TContext>
typename estd::enable_if<
    estd::is_base_of<tags::incoming_context, TContext>::value &&
    estd::is_base_of<UriParserContext, TContext>::value &&
    estd::is_base_of<internal::ExtraContext, TContext>::value, bool>
    ::type
send_sys_reply(TContext& context, typename TContext::encoder_type& encoder)
{
    builder<TContext> build(context, encoder);

    if(build.build_sys_reply())
    {
        context.reply(encoder);
        return true;
    }

    return false;
}


}


}}
