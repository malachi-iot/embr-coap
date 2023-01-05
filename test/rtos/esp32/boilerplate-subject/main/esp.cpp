#include <estd/chrono.h>
#include <estd/istream.h>

#include <embr/observer.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

#include <json/encoder.hpp>

using namespace embr::coap;

#include "context.h"

#include "esp_wifi.h"


namespace sys_paths {


static void build_stats(AppContext& ctx, AppContext::encoder_type& encoder)
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

#if ESP_IDF_VERSION  >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_app_desc_t* app_desc = esp_app_get_description();
#else
    const esp_app_desc_t* app_desc = esp_ota_get_app_description();
#endif

    embr::json::v1::encoder json;
    auto j = make_fluent(json, out);

    j.begin()

    ("uptime", now_in_s.count())
    ("rssi", wifidata.rssi)
    ("versions")
        ("app", app_desc->version)
    --;

    j.end();
}

// TODO: bring in participation of 'extra'/response tracker to help us here
inline bool response_assert(AppContext& context, bool condition, Header::Code fail_code)
{
    if(condition == false)
    {
        return false;
    }

    return true;
}


bool build_sys_reply(AppContext& context, AppContext::encoder_type& encoder)
{
    switch(context.found_node())
    {
        case v1::root:
            //build_reply(context, encoder, Header::Code::NotImplemented);
            build_stats(context, encoder);
            break;

        case v1::root_uptime:
            break;

        case v1::root_reboot:
            response_assert(context,
                context.header().code() == Header::Code::Put,
                Header::Code::BadRequest);

            break;

        case v1::root_version:
            build_app_version_response(context, encoder);
            break;

        default: return false;
    }

    return true;
}

}