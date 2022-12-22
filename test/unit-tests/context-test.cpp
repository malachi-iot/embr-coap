#include <catch.hpp>

#include <embr/observer.h>

#include <coap/encoder/factory.h>
#include <coap/decoder/observer/util.h>

#include "test-context.h"

using namespace embr::coap;

static char buf[128];

ExtraContext::encoder_type make_encoder(ExtraContext& context)
{
    return ExtraContext::encoder_type(buf);
}

namespace uris {

// enum class disables auto cast to int :(
enum uris : int
{
    v1
};

}

internal::UriPathMap uri_map[] =
{
    "v1", uris::v1, MCCOAP_URIPATH_NONE
};

TEST_CASE("context tests", "[context]")
{
    ExtraContext context(uri_map);

    SECTION("encoder")
    {
        auto e = make_encoder_reply(context, Header::Code::BadRequest);

        REQUIRE(context.response_code == Header::Code::BadRequest);
        REQUIRE(context.flags.response_sent == false);
        REQUIRE(context.out.data() == nullptr);

        context.reply(e);

        REQUIRE(context.flags.response_sent == true);
        REQUIRE(context.out.data() == buf);
    }
    SECTION("ExtraObserver")
    {

    }
    SECTION("AutoReplyObserver")
    {
        embr::layer0::subject<UriParserObserver, AutoReplyObserver> s;

        REQUIRE(context.flags.response_sent == false);
        REQUIRE(context.out.data() == nullptr);

        s.notify(event::header(Header(Header::Confirmable, Header::Code::Get)), context);
        s.notify(event::option_start{}, context);
        s.notify(event::option(Option::UriPath), context);  // a null URI, won't get matched (404)
        s.notify(event::option_completed{}, context);

        s.notify(event::completed(false), context);

        REQUIRE(context.flags.response_sent == true);
        REQUIRE(context.response_code == Header::Code::NotFound);
        REQUIRE(context.out.data() == buf);
    }
}