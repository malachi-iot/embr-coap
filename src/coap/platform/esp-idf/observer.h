#pragma once

#include <estd/port/identify_platform.h>

// TODO: use relative pathing here
#include <exp/events.h>
#include "../../../exp/uripath-repeater.h"   // brings in UriPathMatcher and friends

#include "esp_log.h"
#if ESTD_IDF_VER >= ESTD_IDF_VER_3_3_0
#include "esp_ota_ops.h"
#endif

#include "../../context.h"
#include "../../encoder/streambuf.h"

namespace moducom { namespace coap {

// for coap version command request

struct VersionObserverBase
{
    static const char *TAG;
};

// TODO: This belongs in a different file really
template <bool inline_token, class TStreambuf>
void build_version_response(
    const moducom::coap::TokenAndHeaderContext<inline_token>& context, 
    embr::coap::experimental::StreambufEncoder<TStreambuf>& encoder)
{
    typedef moducom::coap::Header Header;
    typedef moducom::coap::Option Option;

    build_reply(context, encoder, Header::Code::Content);

    // text/plain
    encoder.option_int(Option::Numbers::ContentFormat, 0);

    encoder.payload();

    auto out = encoder.ostream();

#if ESTD_IDF_VER >= ESTD_IDF_VER_3_3_0
    const esp_app_desc_t* app_desc = esp_ota_get_app_description();

    out << app_desc->version;
#else
    out << PROJECT_VER;
#endif
}


// Expects TContext to be/conform to:
// moducom::coap::IncomingContext
template <int id_path, class TContext>
struct VersionObserver : VersionObserverBase
{
    typedef moducom::coap::Header Header;
    typedef moducom::coap::Option Option;
    typedef moducom::coap::experimental::completed_event completed_event;

    static void on_notify(completed_event, TContext& ctx) 
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