#include <catch.hpp>

#include <embr/observer.h>

#include <coap/decoder.hpp>
#include <coap/decoder/observer/util.h>
#include <coap/encoder/factory.h>

#include "test-context.h"

using namespace embr::coap;

namespace test {

char synthetic_outbuf[128];

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
    Decoder decoder;
    char* buf = test::synthetic_outbuf;
    estd::span<const uint8_t> _buf((uint8_t*)buf, sizeof(test::synthetic_outbuf));
    Decoder::Context out_chunk(_buf, true);

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

        SECTION("ExtraContext")
        {
            s.notify(event::header(Header(Header::Confirmable, Header::Code::Get)), context);
            s.notify(event::option_start{}, context);
            s.notify(event::option(Option::UriPath), context);  // a null URI, won't get matched (404)
            s.notify(event::option_completed{}, context);

            s.notify(event::completed(false), context);

            REQUIRE(context.flags.response_sent == true);
            REQUIRE(context.response_code == Header::Code::NotFound);
            REQUIRE(context.out.data() == buf);
        }
        SECTION("IncomingContext (UriParserContext only)")
        {
            // For this scenario, we emit a 404 if no matching URI was found.  No checks
            // are done to see if emit already happened, since that requires 'Extra' context
            SyntheticIncomingContext context(uri_map);

            s.notify(event::header(Header(Header::Confirmable, Header::Code::Get)), context);
            s.notify(event::option_start{}, context);
            s.notify(event::option(Option::UriPath), context);  // a null URI, won't get matched (404)
            s.notify(event::option_completed{}, context);

            s.notify(event::completed(false), context);

            REQUIRE(context.out.data() == buf);

            while(!decoder.process_iterate(out_chunk).done);
            REQUIRE(decoder.header_decoder().code() == Header::Code::NotFound);
        }
    }
}