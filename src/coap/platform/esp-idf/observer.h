#pragma once

#include <estd/port/identify_platform.h>

// TODO: use relative pathing here
#include <exp/events.h>
#include "../../../exp/uripath-repeater.h"   // brings in UriPathMatcher and friends

#include "esp_log.h"
// DEBT: Use the Espressif-provided version check here instead, this guy is deprecated
#if ESTD_IDF_VER >= ESTD_IDF_VER_3_3_0
#include "esp_ota_ops.h"
#endif

#include "../../context.h"
#include "../../encoder/streambuf.h"

namespace embr { namespace coap {

// for coap version command request

struct VersionObserverBase
{
    static const char *TAG;
};

// TODO: This belongs in a different file really
template <bool inline_token, class TStreambuf>
void build_app_version_response(
    const embr::coap::TokenAndHeaderContext<inline_token>& context, 
    embr::coap::StreambufEncoder<TStreambuf>& encoder)
{
    typedef embr::coap::Header Header;
    typedef embr::coap::Option Option;

    build_reply(context, encoder, Header::Code::Content);

    // text/plain
    encoder.option(
        Option::Numbers::ContentFormat,
        Option::ContentFormats::TextPlain);

    // NOTE: ember::mem::impl::out_netbuf_streambuf doesn't implement sputc, so this fails
    // Since that is streambuf not the coap encoder, this is a NOTE not a FIX
    bool success = encoder.payload();

    ESP_LOGD("build_version_response", "payload write=%d", success);

    auto out = encoder.ostream();

#if ESP_IDF_VERSION  >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_app_desc_t* app_desc = esp_app_get_description();

    out << app_desc->version;
#elif ESTD_IDF_VER >= ESTD_IDF_VER_3_3_0
    const esp_app_desc_t* app_desc = esp_ota_get_app_description();

    out << app_desc->version;
#elif defined(PROJECT_VER)
    out << PROJECT_VER;
#else
    out << "No version data";
#endif
}


// Expects TContext to be/conform to:
// embr::coap::IncomingContext
template <int id_path, class TContext>
struct VersionObserver : VersionObserverBase
{
    typedef embr::coap::Header Header;
    typedef embr::coap::Option Option;

    static void on_notify(event::completed, TContext& ctx)
    {
        ESP_LOGD(TAG, "on_notify(completed_event)");

        if(ctx.uri_matcher().last_found() == id_path)
        {
            auto encoder = make_encoder(ctx);

            build_version_response(ctx, encoder);

            ctx.reply(encoder);
        }
    }
};


}}