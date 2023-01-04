#include <estd/chrono.h>
#include <estd/istream.h>

#include <embr/observer.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

using namespace embr::coap;

#include "context.h"

#include "esp_wifi.h"

namespace sys_paths {

struct minijson
{
    struct
    {
        uint32_t has_items_ : 8;
        uint32_t level_ : 3;
    };

    minijson() : has_items_(0), level_(0)
    {

    }

    bool has_items() const { return has_items_ >> level_; }
    
    void set_has_items()
    {
        has_items_ |= 1 << level_;
    }

    void clear_has_items()
    {
        has_items_ &= ~(1 << level_);
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        if(has_items()) out << ',';

        set_has_items();

        out << '{';
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void add_key(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        if(has_items()) out << ',';

        set_has_items();

        out << '"' << key << "\":";
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        add_key(out, key);
        out << '{';
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void end(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        clear_has_items();

        out << '}';
        --level_;
    }

    template <class TStreambuf, class TBase>
    void add(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key, const char* value)
    {
        add_key(out, key);
        out << '"' << value << '"';
    }

    template <class TStreambuf, class TBase>
    void add(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key, int value)
    {
        add_key(out, key);
        out << value;
    }
};


static void stats(AppContext& ctx, AppContext::encoder_type& encoder)
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

    out << '{';
    // finally, esp has 64-bit timers to track RTC/uptime.  Pretty
    // sure that's gonna be better than the freertos source
    out <<  "\"uptime\":" << now_in_s.count() << ',';
    out << "\"rssi\":" << wifidata.rssi << ",";
    out << "\"versions\":{";
    out << "\"app\": '" << app_desc->version << "'";
    out << "}";
    out << '}';

    /* works well so far, just have other crashes to consider
    minijson json;

    json.begin(out);
    json.add(out, "uptime", now_in_s.count());
    json.add(out, "rssi", wifidata.rssi);
    json.begin(out, "versions");
    json.add(out, "app", app_desc->version);
    json.add(out, "synthetic", 7);
    json.end(out);
    json.add(out, "synthetic", 100);
    json.end(out);
    */
    
    ctx.reply(encoder);
}



bool build_sys_reply(AppContext& context, AppContext::encoder_type& encoder)
{
    switch(context.found_node())
    {
        case v1::root:
            //build_reply(context, encoder, Header::Code::NotImplemented);
            stats(context, encoder);
            break;

        case v1::root_uptime:
            break;

        case v1::root_reboot:
            break;

        case v1::root_version:
            build_app_version_response(context, encoder);
            break;

        default: return false;
    }

    return true;
}

}