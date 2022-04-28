#include <catch.hpp>

#include "coap/uint.h"

using namespace embr::coap;

TEST_CASE("uint tests", "[uint]")
{
    SECTION("Basic tests")
    {
        uint8_t out[4];

        UInt::set_padded(77, out, sizeof(out));

        REQUIRE(out[0] == 0);
        REQUIRE(out[1] == 0);
        REQUIRE(out[2] == 0);
        REQUIRE(out[3] == 77);

        REQUIRE(UInt::get<uint32_t>(out) == 77);
    }
}
