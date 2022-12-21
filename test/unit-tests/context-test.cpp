#include <catch.hpp>

#include <coap/encoder/factory.h>

#include "test-context.h"

using namespace embr::coap;

static char buf[128];

ExtraContext::encoder_type make_encoder(ExtraContext& context)
{
    return ExtraContext::encoder_type(buf);
}

TEST_CASE("context tests", "[context]")
{
    SECTION("encoder")
    {
        ExtraContext context;

        auto e = make_encoder_reply(context, Header::Code::BadRequest);

        REQUIRE(context.response_code == Header::Code::BadRequest);
        REQUIRE(context.flags.response_sent == false);
        REQUIRE(context.out.data() == nullptr);

        context.reply(e);

        REQUIRE(context.flags.response_sent == true);
        REQUIRE(context.out.data() == buf);
    }
}