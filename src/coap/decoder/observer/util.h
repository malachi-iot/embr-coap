#pragma once

#include <estd/port/identify_platform.h>

#ifdef ESTD_IDF_VER
#include "esp_log.h"
#endif

/**
 * Where all the generic/utility observers go
 * imported from bronze-star project
 * TODO: disambiguate naming for CoAP observe feature vs our subject-observer approach
 * TODO: dissimenate all this utility code into official coap source itself
 * TODO: decouple from esp-idf
 */

#include <coap/context.h>
#include "../uri.h"   // brings in UriPathMatcher and friends
#include "../events.h"

namespace moducom { namespace coap {

// Candidate for 'Provider' since this mostly just holds on the an instance
class UriParserContext
{
    typedef moducom::coap::experimental::UriPathMatcher3 matcher_type;
    typedef matcher_type::optional_int_type optional_int_type;

    // tracks current parsed/requested URI in incoming context
    matcher_type _uri_matcher;

protected:
    typedef moducom::coap::experimental::UriPathMap UriPathMap;
    
    template <int N>
    UriParserContext(const UriPathMap (&paths)[N]) : _uri_matcher(paths) {}

public:
    matcher_type& uri_matcher() { return _uri_matcher; }

    int found_node() const 
    {
        // FIX: auto-casting to int is having some trouble, it's always coming back
        // '1' presumably denoting bool=true for value present

        optional_int_type _found_node = _uri_matcher.last_found();

        return _found_node.value();
    }
};




struct TokenContextObserver : ExperimentalDecoderEventTypedefs
{
    template <bool inline_token>
    static void on_notify(const token_event& e, moducom::coap::TokenContext<inline_token>& ctx) 
    {
        ctx.token(e.chunk);
    }
};

// stateless observer which populates some of the context items
struct HeaderContextObserver : ExperimentalDecoderEventTypedefs
{
    static void on_notify(const header_event& e, moducom::coap::HeaderContext& ctx) 
    {
        ctx.header(e.h);
    }
};

// FIX: Needs better name
struct UriParserObserver : ExperimentalDecoderEventTypedefs
{
    static const char* TAG;

    // TODO: Put this elsewhere in a reusable component
    static void on_notify(const option_event& e, UriParserContext& ctx)
    {
#ifdef ESTD_IDF_VER
        // Only here to help diagnose event-not-firing stuff
        ESP_LOGD(TAG, "on_notify(option_event)=%d", e.option_number);
#endif

        switch(e.option_number)
        {
            case moducom::coap::Option::UriPath:
                ctx.uri_matcher().find(e.string());
                break;

            default:
                break;
        }
    }

#ifdef ESTD_IDF_VER
    static void on_notify(option_completed_event, UriParserContext& ctx)
    {
        ESP_LOGI(TAG, "on_notify(option_completed_event) uri path node=%d", ctx.found_node());
    }
#endif
};

struct _UriBuilderObserver : ExperimentalDecoderEventTypedefs
{
    static const char* TAG;
    static estd::layer1::string<128> incoming_uri;

    static void on_notify(const header_event& e) 
    {
        incoming_uri.clear();
    }

    static void on_notify(const option_event& e) 
    {
        if(e.option_number == moducom::coap::Option::UriPath)
        {
            incoming_uri += e.string();
            incoming_uri += '/';
        }
    }

#ifdef ESTD_IDF_VER
    static void on_notify(option_completed_event)
    {
        // underlying incoming_uri is fixed-alloc null-terminated, so this works
        ESP_LOGI(TAG, "uri path=%s", incoming_uri.data());
    }
#endif
};


// Depends on TContext conforming to/being 
// 1. UriParserContext
// 2. moducom::coap::TokenAndHeaderContext
template <class TContext>
struct Auto404Observer : ExperimentalDecoderEventTypedefs
{
    typedef moducom::coap::Header Header;
    typedef moducom::coap::Option Option;

    static void on_notify(option_completed_event, TContext& context)
    {
#ifdef ESTD_IDF_VER
        ESP_LOGD("Auto404Observer", "on_notify(completed)");
#endif

        if(context.found_node() != MCCOAP_URIPATH_NONE)
            return;

        auto encoder = make_encoder_reply(context, Header::Code::NotFound);

        context.reply(encoder);
    }
};

}}