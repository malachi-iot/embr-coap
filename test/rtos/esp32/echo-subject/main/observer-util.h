#pragma once

#include "esp_log.h"

/**
 * Where all the generic/utility observers go
 * imported from bronze-star project
 * TODO: disambiguate naming for CoAP observe feature vs our subject-observer approach
 * TODO: dissimenate all this utility code into official coap source itself
 * TODO: decouple from esp-idf
 */

#include <exp/uripath-repeater.h>   // brings in UriPathMatcher and friends

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




struct TokenContextObserver
{
    template <bool inline_token>
    static void on_notify(const token_event& e, TokenContext<inline_token>& ctx) 
    {
        //printf("TokenContextObserver::on_notify(token_event)\n");
        ctx.token(e.chunk);
    }
};

// stateless observer which populates some of the context items
struct HeaderContextObserver
{
    static void on_notify(const header_event& e, HeaderContext& ctx) 
    {
        //printf("HeaderContextObserver::on_notify(header_event)\n");
        ctx.header(e.header);
    }
};

// FIX: Needs better name
struct UriParserObserver
{
    static const char* TAG;

    // TODO: Put this elsewhere in a reusable component
    static void on_notify(const option_event& e, UriParserContext& ctx)
    {
        // Only here to help diagnose event-not-firing stuff
        ESP_LOGD(TAG, "on_notify(option_event)=%d", e.option_number);

        switch(e.option_number)
        {
            case Option::UriPath:
                ctx.uri_matcher().find(e.string());
                break;

            default:
                break;
        }
    }

    static void on_notify(option_completed_event, UriParserContext& ctx)
    {
        ESP_LOGI(TAG, "on_notify(option_completed_event) uri path node=%d", ctx.found_node());
    }
};

struct _UriBuilderObserver
{
    static const char* TAG;
    static estd::layer1::string<128> incoming_uri;

    static void on_notify(const header_event& e) 
    {
        incoming_uri.clear();
    }

    static void on_notify(const option_event& e) 
    {
        if(e.option_number == Option::UriPath)
        {
            incoming_uri += e.string();
            incoming_uri += '/';
        }
    }

    static void on_notify(option_completed_event)
    {
        // underlying incoming_uri is fixed-alloc null-terminated, so this works
        ESP_LOGI(TAG, "uri path=%s", incoming_uri.data());
    }
};

