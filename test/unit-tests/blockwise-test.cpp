#include <catch2/catch.hpp>

#include <coap/blockwise.h>

using namespace embr::coap::experimental;

TEST_CASE("Blockwise option encoder/decoder tests", "[blockwise]")
{
    SECTION("decoding")
    {
        SECTION("Simplistic case")
        {
            // NUM of 0, M of 0 (false) and SZX of 1 == 32
            uint8_t option_value[] = {0x1};

            REQUIRE(option_block_decode_szx(option_value) == 32);
            REQUIRE(option_block_decode_num(option_value) == 0);
            REQUIRE(!option_block_decode_m(option_value));
        }
        SECTION("More complex case")
        {
            // NUM of 1, M of 0 (false) and SZX of 0 == 16
            uint8_t option_value[] = {0x10};

            REQUIRE(option_block_decode_szx(option_value) == 16);
            REQUIRE(option_block_decode_num(option_value) == 1);
            REQUIRE(!option_block_decode_m(option_value));
        }
        SECTION("Complex case")
        {
            // NUM of 0x321, M of 1 (true) and SZX of 2 == 64
            uint8_t option_value[] = {0x32, 0x1A};

            REQUIRE(option_block_decode_szx(option_value) == 64);
            REQUIRE(option_block_decode_num(option_value) == 0x321);
            REQUIRE(option_block_decode_m(option_value));
        }
        SECTION("helper struct")
        {
            uint8_t option_value[] = {0x32, 0x1A};
            OptionBlock<uint16_t> decoded;

            decoded.decode(option_value);

            REQUIRE(decoded.size == 64);
        }
    }
    SECTION("encoding")
    {
        uint8_t option_value[3]; // max size is 3

        SECTION("low level")
        {
            int szx = option_block_encode(option_value, 0x321, true, 2);

            REQUIRE(option_value[0] == 0x32);
            REQUIRE(option_value[1] == 0x1A);
            REQUIRE(szx == 2);
        }
        SECTION("helper struct")
        {
            OptionBlock<uint16_t> encoded { 0x321, 64, true };

            encoded.encode(option_value);
        }
    }
}