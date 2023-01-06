#include <estd/chrono.h>
#include <estd/istream.h>

#include <embr/observer.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

#include <json/encoder.hpp>

#include "context.h"

#include "esp_wifi.h"

#include "verify.hpp"

namespace embr { namespace coap {

namespace sys_paths {

// EXPERIMENTAL
template <class TContext>
struct builder
{
    typedef TContext context_type;
    typedef typename context_type::encoder_type encoder_type;

    context_type& context;
    encoder_type& encoder;

    void stats()
    {

    }
};


template <class TContext>
static void build_stats(TContext& ctx, typename TContext::encoder_type& encoder)
{
    auto now = estd::chrono::freertos_clock::now();
    auto now_in_s = estd::chrono::seconds(now.time_since_epoch());

    wifi_ap_record_t wifidata;  // approximately 80 bytes of stack space
    esp_wifi_sta_get_ap_info(&wifidata);

    build_reply(ctx, encoder, Header::Code::Content);

    encoder.option(
        Option::Numbers::ContentFormat,
        (int)Option::ContentFormats::ApplicationJson);

    encoder.payload();

    auto out = encoder.ostream();

    embr::json::v1::encoder json;
    auto j = make_fluent(json, out);

    j.begin()

    ("uptime", now_in_s.count())
    ("rssi", wifidata.rssi);

    j.end();
}

template <class TContext>
static void build_firmware_info(TContext& ctx, typename TContext::encoder_type& encoder)
{
    build_reply(ctx, encoder, Header::Code::Content);

    encoder.option(
        Option::Numbers::ContentFormat,
        (int)Option::ContentFormats::ApplicationJson);

    encoder.payload();

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
    ("versions")
        ("app", app_desc->version)
        ("idf", app_desc->idf_ver)
    --;

    j.end();
}


template <class TContext>
typename estd::enable_if<
    estd::is_base_of<tags::incoming_context, TContext>::value &&
    estd::is_base_of<UriParserContext, TContext>::value &&
    estd::is_base_of<internal::ExtraContext, TContext>::value, bool>
    ::type
build_sys_reply(TContext& context, typename TContext::encoder_type& encoder)
{
    bool verified;

    switch(context.found_node())
    {
        case v1::root:
            //build_reply(context, encoder, Header::Code::NotImplemented);
            if(!verify(context, Header::Code::Get)) return false;
            
            build_stats(context, encoder);

            break;

        //case v1::root_uptime:
            //break;

        case v1::root_reboot:
            verified = verify(context, Header::Code::Put);

            // Abort, but indicate we did technically service the URI
            if(!verified) return true;

            break;

        case v1::root_firmware:
            if(!verify(context, Header::Code::Get)) return false;

            build_firmware_info(context, encoder);
            break;

        default: return false;
    }

    return true;
}


template <class TContext>
typename estd::enable_if<
    estd::is_base_of<tags::incoming_context, TContext>::value &&
    estd::is_base_of<UriParserContext, TContext>::value &&
    estd::is_base_of<internal::ExtraContext, TContext>::value, bool>
    ::type
send_sys_reply(TContext& context, typename TContext::encoder_type& encoder)
{
    if(build_sys_reply(context, encoder))
    {
        context.reply(encoder);
        return true;
    }

    return false;
}


}

}}
