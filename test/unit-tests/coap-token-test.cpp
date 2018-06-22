//
// Created by malachi on 12/29/17.
//

#include <catch.hpp>
#include "coap-token.h"
#include "coap-blockwise.h"

#include <estd/vector.h>

TEST_CASE("CoAP token tests", "[coap-token]")
{
    SECTION("UInt tests")
    {
        SECTION("Test 1")
        {
            //moducom::pipeline::layer2::MemoryChunk<8> chunk;
            estd::layer1::vector<uint8_t, 8> chunk;

            int bytes_used = moducom::coap::UInt::set(0xFEDCBA, chunk);

            REQUIRE(bytes_used == 3);
            REQUIRE((uint8_t)chunk[0] == 0xFE);
        }
        SECTION("Test 2")
        {
            moducom::coap::layer2::UInt<> uint_val;

            uint_val.set(0xFEDCBA);

            REQUIRE(uint_val.length() == 3);
            REQUIRE(uint_val[0] == 0xFE);

            //uint32_t val = uint_val.get_uint32_t();
            const uint8_t* data = uint_val.data();

            uint32_t val = moducom::coap::UInt::get<uint32_t>(data, uint_val.length());

            REQUIRE(val == 0xFEDCBA);
        }
        SECTION("Test 3")
        {
            //moducom::coap::layer2::UInt uint_val;
        }
    }
    SECTION("Token Generator")
    {
        moducom::coap::SimpleTokenGenerator generator;
        moducom::coap::layer2::Token token;

        generator.generate(&token);
        generator.generate(&token);

        generator.set(0x101);

        generator.generate(&token);

        REQUIRE(token.size() == 2);

        generator.set(0x30201);

        generator.generate(&token);

        REQUIRE((int)token[0] == 3);
        REQUIRE((int)token[1] == 2);
        REQUIRE((int)token[2] == 1);
        REQUIRE(token.size() == 3);
    }
    SECTION("Token test")
    {
        moducom::coap::layer2::Token token;

        //token.set(0);
        token.resize(1);

        token[0] = '3';

        REQUIRE((char)token[0] == '3');
    }
    SECTION("layer3 token")
    {
        moducom::coap::layer2::Token token;

        token.resize(1);
        //token.set(0);

        token[0] = '3';

        REQUIRE((char)token[0] == '3');

        moducom::coap::layer3::Token token2(token);

        REQUIRE(token2 == token);
    }
}
