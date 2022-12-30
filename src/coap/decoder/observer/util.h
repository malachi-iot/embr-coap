#pragma once

#include <estd/port/identify_platform.h>
#include <estd/type_traits.h>

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
#include "../events.h"
#include "../../../exp/events.h"
#include "../../fwd.h"
#include "uri.h"

namespace embr { namespace coap {


struct TokenContextObserver : ExperimentalDecoderEventTypedefs
{
    template <bool inline_token>
    static void on_notify(const token_event& e, embr::coap::TokenContext<inline_token>& ctx) 
    {
        ctx.token(e.chunk);
    }
};

// stateless observer which populates some of the context items
struct HeaderContextObserver
{
    inline static void on_notify(const event::header& e, embr::coap::HeaderContext& ctx)
    {
        ctx.header(e.h);
    }
};

// Builds a full incoming_uri string.  Usually you'll prefer UriParserObserver
// DEBT: Not thread safe!
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
        if(e.option_number == embr::coap::Option::UriPath)
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
// 2. embr::coap::TokenAndHeaderContext
template <class TContext>
struct Auto404Observer : ExperimentalDecoderEventTypedefs
{
    typedef embr::coap::Header Header;
    typedef embr::coap::Option Option;

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

// Generates either 404 or specific response codes, depending on context
// DEBT: Minimally working, but confusing that it depends on 'ExtraContext'
struct AutoReplyObserver : ExperimentalDecoderEventTypedefs
{
    static constexpr const char* TAG = "AutoReplyObserver";

    template <class TContext>
    static void do_404(TContext& context)
    {
#ifdef ESTD_IDF_VER
        ESP_LOGD(TAG, "do_404: entry");
#endif

        if(context.found_node() != MCCOAP_URIPATH_NONE)
            return;

        auto encoder = make_encoder_reply(context, Header::Code::NotFound);

        context.reply(encoder);
    }


    template <class TContext, typename estd::enable_if<
        !estd::is_base_of<internal::ExtraContext, TContext>::value &&
        //estd::is_base_of<HeaderContext, TContext>::value
        true
        , int>::type = 0>
    static void on_notify(event::option_completed, TContext& context)
    {
    }


    template <class TContext, typename estd::enable_if<
        estd::is_base_of<internal::ExtraContext, TContext>::value &&
        estd::is_base_of<UriParserContext, TContext>::value
        //true
        , int>::type = 0>
    static void on_notify(event::option_completed, TContext& context)
    {
        if(context.found_node() == MCCOAP_URIPATH_NONE)
        {
            context.response_code = Header::Code::NotFound;
        }
    }

    template <class TContext, typename estd::enable_if<
            !estd::is_base_of<internal::ExtraContext, TContext>::value &&
            estd::is_base_of<UriParserContext, TContext>::value
            , int>::type = 0>
    static void on_notify(event::completed, TContext& context)
    {
        do_404(context);
    }


    template <class TContext, typename estd::enable_if<
            estd::is_base_of<internal::ExtraContext, TContext>::value &&
            estd::is_base_of<tags::header_context, TContext>::value
            , int>::type = 0>
    static void on_notify(event::completed, TContext& context)
    {
        const Header header = context.header();

        if(context.response_code.has_value() &&
            context.flags.response_sent == false &&
            header.type() == header::Types::Confirmable)
        {
            auto encoder = make_encoder_reply(context, context.response_code.value());

            context.reply(encoder);
        }
    }
};


}}