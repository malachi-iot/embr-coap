#include <catch2/catch.hpp>
#include "coap-encoder.h"

#include <coap/internal/rfc7641/registrar.h>

#include "coap/token.h"
#include "coap/uint.h"
#include "test-data.h"

using namespace embr::coap;
//using namespace moducom::pipeline;

TEST_CASE("CoAP observable (RFC-7641) tests", "[coap-observer]")
{
    typedef estd::span<const uint8_t> ro_chunk_t;

    SECTION("(over)simplest example")
    {
        ro_chunk_t chunk(buffer_oversimplified_observe);
        Decoder d;
        Decoder::Context context(chunk, true);

        d.process_iterate(context);
        d.process_iterate(context);

        REQUIRE(d.state() == Decoder::HeaderDone);

        d.process_iterate(context);

        REQUIRE(d.state() == Decoder::TokenDone);

        d.process_iterate(context);
        d.process_iterate(context);

        REQUIRE(d.state() == Decoder::Options);

        REQUIRE(d.option_number() == Option::Observe);
        REQUIRE(d.option_length() == 0);

        ro_chunk_t value = context.remainder();

        // CoAP folks were clever, a subscribe option only takes one extra byte
        // due to implicit value of their UInt
        uint16_t option_value = UInt::get<uint16_t>(value);

        // TODO: still working on processing this scenario right
        REQUIRE(option_value == 0);
    }
    SECTION("v2")
    {
        embr::coap::layer2::Token token;
        internal::observable::layer1::Registrar<int, 10> registrar;
        internal::observable::layer1::Registrar<int, 10>::key_type key(0, token);

        registrar.add(key, 1);
    }
}
