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

using sic_type = SyntheticIncomingContext<true>;

class subcontext1 : public internal::v1::CoapSubcontextBase::base<sic_type>
{
    using base_type = internal::v1::CoapSubcontextBase::base<sic_type>;

public:
    subcontext1(sic_type& c) : base_type(c) {}

    static constexpr int id_path = 1;
};


template <class Context = sic_type>
class subcontext2 : public internal::v1::CoapSubcontextBase::base<Context>
{
    using base_type = internal::v1::CoapSubcontextBase::base<Context>;
    using encoder_type = typename Context::encoder_type;

public:
    using context_type = Context;

    constexpr explicit subcontext2(Context& c) : base_type(c) {}

    static constexpr int id_path = 1;
    bool completed_ = false;

    bool completed(encoder_type&)
    {
        completed_ = true;
        return true;
    }
};


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
    SECTION("subcontext")
    {
        subcontext::CoapSubcontext<
            subcontext1,
            subcontext2<> > sc;

        auto encoder = make_encoder(context);
        sc.on_completed(encoder, context);

        REQUIRE(sc.state().index() == 0);

        // Remember, we have a 'dummy' unknown at position 0
        sc.state().emplace<2>(context);
        REQUIRE(sc.state().index() == 2);

        REQUIRE(!estd::get<2>(sc.state()).completed_);

        sc.on_completed(encoder, context);

        REQUIRE(estd::get<2>(sc.state()).completed_);
    }
}