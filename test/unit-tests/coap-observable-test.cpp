#include <catch.hpp>
#include "coap-encoder.h"
#include "mc/memory.h"
#include "coap_transmission.h"
#include "coap-token.h"
#include "mc/experimental.h"
#include "coap-observable.h"
#include "coap-uint.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::pipeline;

TEST_CASE("CoAP observable (RFC-7641) tests", "[coap-observer]")
{
    typedef estd::const_buffer ro_chunk_t;

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
        uint16_t option_value = UInt::get<uint16_t>(value.data(), value.size());

        // TODO: still working on processing this scenario right
        REQUIRE(option_value == 0);
    }
}
