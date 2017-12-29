//
// Created by malachi on 12/29/17.
//

#include <catch.hpp>
#include "../coap-token.h"
#include "../coap-blockwise.h"

TEST_CASE("CoAP token tests", "[coap-token]")
{
    SECTION("UInt tests")
    {
        moducom::pipeline::layer2::MemoryChunk<8> chunk;

        int bytes_used = moducom::coap::experimental::OptionUInt::set_uint32_t(0xFEDCBA, chunk);

        REQUIRE(bytes_used == 3);
        REQUIRE(chunk[0] == 0xFE);
    }
    SECTION("1")
    {
        moducom::coap::SimpleTokenGenerator generator;
        moducom::coap::layer2::Token token;

        generator.generate(&token);
        generator.generate(&token);

        generator.set(0x101);

        generator.generate(&token);

        REQUIRE(token._length() == 2);

        generator.set(0x30201);

        generator.generate(&token);

        REQUIRE(token[0] == 3);
        REQUIRE(token[1] == 2);
        REQUIRE(token[2] == 1);
        REQUIRE(token._length() == 3);
    }
}