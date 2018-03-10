#include <catch.hpp>

#include "../cbor.h"

using namespace moducom;

TEST_CASE("CBOR decoder tests", "[cbor-decoder]")
{
    SECTION("Basic test 1")
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
}
