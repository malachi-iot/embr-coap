#include <catch2/catch.hpp>

#include <embr/observer.h>

#include <coap/decoder.hpp>
#include <coap/decoder/observer/util.h>
#include <coap/encoder/factory.h>

#include "test-context.h"

using namespace embr::coap;

namespace test {

char synthetic_outbuf[128];

}


internal::UriPathMap uri_map[] =
{
    { "v1", test::uris::v1, MCCOAP_URIPATH_NONE }
};

#if __cpp_concepts
template <concepts::Functor<bool> F>
void functor_test(F f)
{
    f();
}
#endif


TEST_CASE("context tests", "[context]")
{
    SyntheticIncomingContext<true> context(uri_map);
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
            SyntheticIncomingContext<true> context(uri_map);

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
#if FEATURE_EMBR_COAP_SUBCONTEXT
    SECTION("subcontext")
    {
        subcontext::Subcontext<
            test::subcontext1,
            test::subcontext2<> > sc;

        auto encoder = make_encoder(context);
        sc.on_completed(encoder, context);

        REQUIRE(sc.state().index() == 0);

        // Remember, we have a 'dummy' unknown at position 0
        // TODO: Do a 'create' here rather than peering inside with emplace
        sc.state().emplace<2>(context);
        REQUIRE(sc.state().index() == 2);

        const test::subcontext2<>& sub = estd::get<2>(sc.state());

        REQUIRE(!sub.completed_);
        REQUIRE(sub.value_ == -1);

        constexpr char query[] = "key=123";
        estd::span<const uint8_t> query_binary((const uint8_t*)query, sizeof(query));

        event::option o(Option::UriPath, query_binary, true);

        sc.on_uri_query(o, context);
        sc.on_completed(encoder, context);

#if __cpp_generic_lambdas >= 201707L
        REQUIRE(sub.value_ == 123);
        REQUIRE(estd::get<2>(sc.state()).completed_);
#endif
    }
#endif
#if __cpp_concepts
    SECTION("experimental")
    {
        int val = 0;

        functor_test([&](){ ++val; return true; });

        REQUIRE(val == 1);
    }
#endif
}
