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

inline namespace v1 {

struct textfmt
{
    bool use_eol()      { return true; }
    bool use_tabs()     { return true; }
    bool use_spaces()   { return true; }
    bool brace_on_newline() { return true; }
};

struct minijson : textfmt
{
    struct
    {
        uint32_t has_items_ : 8;
        uint32_t level_ : 3;
        uint32_t in_array_ : 1;
    };

    minijson() : has_items_(0), level_(0), in_array_(0)
    {

    }

    template <class TStreambuf, class TBase>
    inline void do_tabs(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        if(use_tabs())
        {
            for(unsigned i = level_; i > 0; --i)
                out << "  ";
        }
    }

    template <class TStreambuf, class TBase>
    inline void do_eol(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        if(use_eol()) out << estd::endl;
    }

    bool has_items() const { return has_items_ >> level_; }
    bool in_array() const { return in_array_; }
    
    void set_has_items()
    {
        has_items_ |= 1 << level_;
    }

    // clear out indicator that this level has items
    // (do this when moving back up a level)
    void clear_has_items()
    {
        has_items_ &= ~(1 << level_);
    }

    // Prints comma and eol if items preceded this
    template <class TStreambuf, class TBase>
    void do_has_items(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        if(has_items())
        {
            out << ',';
            do_eol(out);
        }
        else
            set_has_items();
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        do_tabs(out);
        do_has_items(out);

        out << '{';
        do_eol(out);
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void add_key(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        do_has_items(out);

        do_tabs(out);
        
        out << '"' << key << "\":";

        if(use_spaces()) out << ' ';
    }

    template <class TStreambuf, class TBase>
    void begin(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        add_key(out, key);

        if(brace_on_newline())
        {
            do_eol(out);
            do_tabs(out);
        }

        out << '{';
        do_eol(out);

        ++level_;
    }

    template <class TStreambuf, class TBase>
    void array(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key)
    {
        in_array_ = 1;

        add_key(out, key);

        out << '[';
        ++level_;
    }

    template <class TStreambuf, class TBase>
    void raw(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* value)
    {
        out << '"' << value << '"';
    }

    template <class TStreambuf, class TBase>
    void raw(estd::internal::basic_ostream<TStreambuf, TBase>& out, int value)
    {
        out << value;
    }

    template <class TStreambuf, class TBase, typename T>
    void array_item(estd::internal::basic_ostream<TStreambuf, TBase>& out, T value)
    {
        if(has_items())
        {
            out << ',';
            if(use_spaces()) out << ' ';
        }
        else
            set_has_items();

        raw(out, value);
    }

    template <class TStreambuf, class TBase>
    void end_array(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        out << ']';
        --level_;
        in_array_ = 0;
    }

    template <class TStreambuf, class TBase>
    void end(estd::internal::basic_ostream<TStreambuf, TBase>& out)
    {
        clear_has_items();

        if(in_array())
        {
            end_array(out);
            return;
        }

        --level_;

        do_eol(out);
        do_tabs(out);

        out << '}';
    }

    template <class TStreambuf, class TBase, typename T>
    void add(estd::internal::basic_ostream<TStreambuf, TBase>& out, const char* key, T value)
    {
        add_key(out, key);
        raw(out, value);
    }
};


namespace minij {

enum modes
{
    core = 0,
    array,
    normal,
    begin
};

}

template <class TOut, int mode = minij::core>
struct minijson_fluent;



template <class TStreambuf, class TBase, int mode_>
struct minijson_fluent<estd::detail::basic_ostream<TStreambuf, TBase>, mode_>
{
    typedef estd::detail::basic_ostream<TStreambuf, TBase> out_type;

    typedef minijson_fluent<out_type> default_type;
    typedef minijson_fluent<out_type, mode_> this_type;
    typedef minijson_fluent<out_type, minij::array> array_type;
    typedef minijson_fluent<out_type, minij::normal> mode2_type;
    typedef minijson_fluent<out_type, minij::begin> begin_type;

    static ESTD_CPP_CONSTEXPR_RET int mode() { return mode_; }

    out_type& out;
    minijson& json;

    minijson_fluent(out_type& out, minijson& json) :
        out(out),
        json(json)
    {}

    template <typename T>
    minijson_fluent& add(const char* key, T value)
    {
        json.add(out, key, value);
        return *this;
    }

    minijson_fluent& begin()
    {
        json.begin(out);
        return *this;
    }

    minijson_fluent& begin(const char* key)
    {
        json.begin(out, key);
        return *this;
    }

    minijson_fluent& array(const char* key)
    {
        json.array(out, key);
        return *this;
    }

    template <class T>
    minijson_fluent& array_item(T value)
    {
        json.array_item(out, value);
        return *this;
    }

    minijson_fluent& end()
    {
        json.end(out);
        return *this;
    }

    minijson_fluent& operator()(const char* key)
    {
        return begin(key);
    }

    minijson_fluent& operator()()
    {
        return end();
    }

    template <typename T>
    minijson_fluent& operator()(const char* key, T value)
    {
        return add(key, value);
    }

    array_type& operator[](const char* key)
    {
        return (array_type&) array(key);
    }

    minijson_fluent& operator--(int)
    {
        return end();
    }

/*
 - fascinating, but deeper overloading of () is better
    template <typename T>
    minijson_fluent& operator,(T value)
    {
        json.array_item(out, value);
        return *this;
    } */

    // almost but not quite
    /*
    begin_type& operator++(int)
    {
        return (begin_type&) *this;
    } */


/*
    begin_type* operator->()
    {
        return (begin_type*) this;
    } */

    /*
    minijson_fluent& operator=(const char* key)
    {
        return *this;
    } */
};

// TODO: Look into that fnptr-like magic that ostream uses for its endl
template <class TOut, int mode>
minijson_fluent<TOut, mode>& end(minijson_fluent<TOut, mode>& j)
{
    return j.end();
}

template <class TStreambuf, class TBase>
struct minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase>, minij::array> :
    minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase> >
{
    typedef minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase> > base_type;
    typedef minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase>, minij::array> this_type;

    template <class T>
    void array_items(T item)
    {
        base_type::array_item(item);
    }

    template <class T, class ... TArgs>
    void array_items(T item, TArgs...args)
    {
        base_type::array_item(item);
        array_items(std::forward<TArgs>(args)...);
    }

    template <class ... TArgs>
    base_type& operator()(TArgs...args)
    {
        array_items(std::forward<TArgs>(args)...);
        base_type::end();
        return *this;
    }
};

template <class TStreambuf, class TBase>
struct minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase>, minij::begin> :
    minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase> >
{
    minijson_fluent& operator=(const char* key)
    {
        return *this;
    }
};


template <class TOut>
minijson_fluent<TOut>& operator<(minijson_fluent<TOut>& j, const char* key)
{
    return j.begin(key);
}

// doesn't work
template <class TOut>
minijson_fluent<TOut>& operator>(minijson_fluent<TOut>& j, minijson_fluent<TOut>&)
{
    return j.end();
}


template <class TStreambuf, class TBase>
auto make_fluent(estd::internal::basic_ostream<TStreambuf, TBase>& out, minijson& json)
{
    return minijson_fluent<estd::internal::basic_ostream<TStreambuf, TBase> >(out, json);
}

}

namespace sys_paths {


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

    /*
    out << '{';
    // finally, esp has 64-bit timers to track RTC/uptime.  Pretty
    // sure that's gonna be better than the freertos source
    out <<  "\"uptime\":" << now_in_s.count() << ',';
    out << "\"rssi\":" << wifidata.rssi << ",";
    out << "\"versions\":{";
    out << "\"app\": '" << app_desc->version << "'";
    out << "}";
    out << '}'; */

    ::v1::minijson json;
    auto j = make_fluent(out, json);

    j.begin();
    j.add("uptime", now_in_s.count());
    j("rssi", wifidata.rssi);
    j.begin("versions").
        add("app", app_desc->version)
        ("nested")
            ("n1", 123)
            ("n2", 789)
        --
        ("synthetic", 7)
    --;

    // doesn't quite work
    //j < "nested2" ("n2", 456) >j;
    //j = "nested2" ("n2", 456) --;

    j["ar1"] (213, 867, 5309);
    j["ar2"] (900, 976, 1234);

    j.add("synthetic", 100);
    json.end(out);
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