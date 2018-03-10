#include <catch.hpp>

#include "../cbor.h"

using namespace moducom;

TEST_CASE("CBOR decoder tests", "[cbor-decoder]")
{
    SECTION("True/false test")
    {
        CBOR::Decoder decoder;
        uint8_t value = 0b11100000 | 21;

        do
        {
            decoder.process(value);
        }
        while(decoder.state() != CBOR::Decoder::Done);

        REQUIRE(decoder.is_simple_type_bool() == true);
    }
    SECTION("16-bit integer test")
    {
        CBOR::Decoder decoder;
        uint8_t value[] = { CBOR::Decoder::bits_16, 0xF0, 0x77 };
        uint8_t* v= value;

        do
        {
            decoder.process(*v++);
        }
        while(decoder.state() != CBOR::Decoder::Done);

        REQUIRE(decoder.is_simple_type_bool() == false);
        REQUIRE(decoder.type() == CBOR::PositiveInteger);
        REQUIRE(decoder.is_16bit());
        uint16_t retrieved = decoder.value<uint16_t>();
        REQUIRE(retrieved == 0xF077);
    }
}
