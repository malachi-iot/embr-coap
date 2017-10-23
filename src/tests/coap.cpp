
#include <catch.hpp>

#include "../coap.h"

using namespace moducom::coap;

TEST_CASE("CoAP tests", "[coap]")
{
    SECTION("Basic header construction")
    {
        CoAP::Header header(CoAP::Header::Confirmable);

        REQUIRE(header.raw == 0x40000000);

        header.type(CoAP::Header::NonConfirmable);

        REQUIRE(header.raw == 0x50000000);
    }
}
