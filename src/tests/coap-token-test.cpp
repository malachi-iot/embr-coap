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
        SECTION("Test 1")
        {
            moducom::pipeline::layer2::MemoryChunk<8> chunk;

            int bytes_used = moducom::coap::UInt::set(0xFEDCBA, chunk);

            REQUIRE(bytes_used == 3);
            REQUIRE(chunk[0] == 0xFE);
        }
        SECTION("Test 2")
        {
            moducom::coap::layer2::UInt uint_val;

            uint_val.set(0xFEDCBA);

            REQUIRE(uint_val.length() == 3);
            REQUIRE(uint_val[0] == 0xFE);

            //uint32_t val = uint_val.get_uint32_t();

            uint32_t val = moducom::coap::UInt::get<uint32_t>(uint_val.data(), uint_val.length());

            REQUIRE(val == 0xFEDCBA);
        }
        SECTION("Test 3")
        {
            //moducom::coap::layer2::UInt uint_val;
        }
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