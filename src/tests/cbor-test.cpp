#include <catch.hpp>

#include <string.h>
#include "../cbor.h"
#include "../cbor/encoder.h"

using namespace moducom;

static uint8_t cbor_cred[] =
{
        0xA2,                       // map(2)
0x64,                    // text(4)
0x73, 0x73, 0x69, 0x64,          // "ssid"
0x69,                    // text(9)
0x73, 0x73, 0x69, 0x64, 0x5F, 0x6E, 0x61, 0x6D, 0x65, // "ssid_name"
0x64,                    // text(4)
0x70, 0x61, 0x73, 0x73,           // "pass"
0x66,                    // text(6)
0x73, 0x65, 0x63, 0x72, 0x65, 0x74        // "secret"
};

static uint8_t cbor_int[] =
        {
                0xA1,             // map(1)
        0x63,          // text(3)
        0x76, 0x61, 0x6C,   // "val"
        0x3A, 0x07, 0x5B, 0xCD, 0x14 // negative(123456788)
        };


static const uint8_t* assert(CBOR::Decoder& decoder, const uint8_t* v, std::string expected)
{
    REQUIRE(decoder.type() == CBOR::String);
    REQUIRE(decoder.get_simple_value() == expected.length());
    std::string s((const char*)v, expected.length());

    REQUIRE(s == expected);

    v += expected.length();
    return v;
}


static std::string decoder_get_string(CBOR::Decoder& decoder, const uint8_t** v)
{
    pipeline::MemoryChunk::readonly_t chunk = decoder.get_string_experimental(v, 999);
    std::string s((const char*)chunk.data(), chunk.length());
    return s;
}


#ifdef __APPLE__
#define __glibcxx_assert(dummy)
#elif defined(_MSC_VER)
#define __glibcxx_assert(dummy)
#else
#endif

template <class TDecoder = CBOR::Decoder>
struct DecoderHelper
{
    TDecoder decoder;
    // TODO: Need to refactor whole thing to take chunks so that we can more sensibly
    // manage max_len and its diminishing size
    const uint8_t* value;
    //size_t max_len;
    CBOR::Decoder::ParseResult result;

    DecoderHelper(const uint8_t* value) : value(value)
    {}

    DecoderHelper(TDecoder& decoder, const uint8_t* value) :
            decoder(decoder),
            value(value)
    {}

    std::string string()
    {
        pipeline::MemoryChunk::readonly_t chunk = decoder.get_string_experimental(&value, 999, &result);
        __glibcxx_assert(result == CBOR::Decoder::OK);
        std::string s((const char*)chunk.data(), chunk.length());
        return s;
    }

    int integer()
    {
        int ret_val = decoder.get_int_experimental(&value, 999, &result);
        __glibcxx_assert(result == CBOR::Decoder::OK);
        return  ret_val;
    }

    int map()
    {
        int ret_val = decoder.get_map_experimental(&value, 999, &result);
        __glibcxx_assert(result == CBOR::Decoder::OK);
        return  ret_val;
    }
};

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
    SECTION("Synthetic cred-set test: very manual")
    {
        CBOR::Decoder decoder;
        const uint8_t* v = cbor_cred;

        v = decoder.process(v);

        REQUIRE(decoder.type() == CBOR::Map);
        uint8_t len = decoder.get_simple_value();
        REQUIRE(len == 2);

        v = decoder.process(v);
        v = assert(decoder, v, "ssid");

        v = decoder.process(v);
        v = assert(decoder, v, "ssid_name");

        v = decoder.process(v);
        v = assert(decoder, v, "pass");

        v = decoder.process(v);
        v = assert(decoder, v, "secret");
    }
    SECTION("Synthetic cred-set test: with helpers")
    {
        CBOR::Decoder decoder;
        const uint8_t* v = cbor_cred;

        REQUIRE(decoder.get_map_experimental(&v, 999) == 2);
        REQUIRE(decoder_get_string(decoder, &v) == "ssid");
        REQUIRE(decoder_get_string(decoder, &v) == "ssid_name");
        REQUIRE(decoder_get_string(decoder, &v) == "pass");
        REQUIRE(decoder_get_string(decoder, &v) == "secret");

        v = cbor_cred;
        DecoderHelper<CBOR::Decoder&> dh(decoder, v);

        REQUIRE(dh.map() == 2);
        REQUIRE(dh.string() == "ssid");
        REQUIRE(dh.string() == "ssid_name");
        REQUIRE(dh.string() == "pass");
        REQUIRE(dh.string() == "secret");
    }
    SECTION("int decoder")
    {
        DecoderHelper<> dh(cbor_int);

        REQUIRE(dh.map() == 1);
        REQUIRE(dh.string() == "val");
        REQUIRE(dh.integer() == -123456789);
    }
    SECTION("basic encoder: layer1")
    {
        cbor::layer1::Encoder encoder;

        encoder.integer(10);

        REQUIRE(encoder.data()[0] == 10);

        encoder.integer(100);
        encoder.integer(1000);
    }
    SECTION("basic encoder: layer3")
    {
        uint8_t buffer[9];
        cbor::layer3::Encoder encoder(buffer);

        encoder.integer(10);

        REQUIRE(encoder.data()[0] == 10);

        encoder.integer(100);

        REQUIRE(encoder.data()[0] == CBOR::Decoder::bits_8);

        encoder.integer(1000);
    }
    SECTION("more advanced encoder: layer1")
    {
        cbor::layer1::Encoder encoder;

        encoder.integer(-123456789);

        REQUIRE(encoder.data()[0] == (CBOR::NegativeInteger << 5 | CBOR::Decoder::bits_32));

        int len = encoder.string(5);

        REQUIRE(len == 1);
        REQUIRE(encoder.data()[0] == (CBOR::String << 5 | 5));
    }
}
