#pragma once

#include <estd/port/identify_platform.h>
#include <estd/type_traits.h>

#ifdef ESTD_IDF_VER
#include "esp_log.h"
#endif

#include "../uri.h"   // brings in UriPathMatcher and friends
#include "../events.h"

namespace embr { namespace coap {

// Candidate for 'Provider' since this mostly just holds on the an instance
class UriParserContext
{
    typedef embr::coap::internal::UriPathMatcher matcher_type;
    typedef matcher_type::optional_int_type optional_int_type;

    // tracks current parsed/requested URI in incoming context
    matcher_type _uri_matcher;

protected:
    typedef embr::coap::internal::UriPathMap UriPathMap;

    template <int N>
    UriParserContext(const UriPathMap (&paths)[N]) : _uri_matcher(paths) {}

public:
    matcher_type& uri_matcher() { return _uri_matcher; }

    int found_node() const
    {
        // FIX: auto-casting to int is having some trouble, it's always coming back
        // '1' presumably denoting bool=true for value present

        optional_int_type _found_node = _uri_matcher.last_found();

        return *_found_node;
    }
};


// Parses and matches incoming URI via UriPathMatcher
// FIX: Needs better name
struct UriParserObserver
{
    static const char* TAG;

    // TODO: Put this elsewhere in a reusable component
    static void on_notify(const event::option& e, UriParserContext& ctx)
    {
#ifdef ESTD_IDF_VER
        // Only here to help diagnose event-not-firing stuff
        ESP_LOGD(TAG, "on_notify(option)=%d", e.option_number);
#endif

        switch(e.option_number)
        {
            case embr::coap::Option::UriPath:
                ctx.uri_matcher().find(e.string());
                break;

            default:
                break;
        }
    }

#ifdef ESTD_IDF_VER
    static void on_notify(event::option_completed, UriParserContext& ctx)
    {
        ESP_LOGI(TAG, "on_notify(option_completed) uri path node=%d", ctx.found_node());
    }
#endif
};


}}