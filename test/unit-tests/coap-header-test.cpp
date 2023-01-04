#include <catch2/catch.hpp>

#include "coap/header.h"
#include "test-data.h"

using namespace embr::coap;

TEST_CASE("CoAP header tests", "[coap-header]")
{
    SECTION("Basic header construction")
    {
        Header header(Header::Confirmable);

        REQUIRE(header.type() == Header::Confirmable);
        REQUIRE(header.bytes[0] == 0x40);
        REQUIRE(header.bytes[1] == 0);
        REQUIRE(header.bytes[2] == 0);
        REQUIRE(header.bytes[3] == 0);

        header.type(Header::NonConfirmable);

        REQUIRE(header.type() == Header::NonConfirmable);
        REQUIRE(header.bytes[0] == 0x50);
        REQUIRE(header.bytes[1] == 0);
        REQUIRE(header.bytes[2] == 0);
        REQUIRE(header.bytes[3] == 0);
    }
    SECTION("Variants")
    {
        SECTION("c++03")
        {
            internal::cxx_03::Header header;
        }
        SECTION("c++11")
        {
            internal::cxx_11::Header header;

            estd::copy_n(buffer_ack, 4, header.begin());

            REQUIRE(header.type() == header::Types::Acknowledgement);
            REQUIRE(header.version() == 1);
            REQUIRE(header.token_length() == 0);

            REQUIRE(header.data()[0] == 0x60);
        }
    }
}