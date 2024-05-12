#include <catch2/catch.hpp>

#include <coap/blockwise.h>

#include "test-context.h"

using namespace embr::coap::experimental;

TEST_CASE("Blockwise option encoder/decoder tests", "[blockwise]")
{
    SECTION("decoding")
    {
        SECTION("Simplistic case")
        {
            // NUM of 0, M of 0 (false) and SZX of 1 == 32
            uint8_t option_value[] = {0x1};

            REQUIRE(option_block_decode_szx_to_size(option_value) == 32);
            REQUIRE(option_block_decode_num(option_value) == 0);
            REQUIRE(!option_block_decode_m(option_value));
        }
        SECTION("More complex case")
        {
            // NUM of 1, M of 0 (false) and SZX of 0 == 16
            uint8_t option_value[] = {0x10};

            REQUIRE(option_block_decode_szx_to_size(option_value) == 16);
            REQUIRE(option_block_decode_num(option_value) == 1);
            REQUIRE(!option_block_decode_m(option_value));
        }
        SECTION("Complex case")
        {
            // NUM of 0x321, M of 1 (true) and SZX of 2 == 64
            uint8_t option_value[] = {0x32, 0x1A};

            REQUIRE(option_block_decode_szx_to_size(option_value) == 64);
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
    SECTION("state machine")
    {
        char buffer[512];

        span_encoder_type encoder(buffer);

        embr::coap::Header header(
            embr::coap::Header::Acknowledgement,
            embr::coap::Header::Code::Continue);

        header.message_id(0x1234);

        encoder.header(header);

        auto offset = encoder.rdbuf()->pubseekoff(0, estd::ios_base::cur);
        REQUIRE(offset == 4);

        BlockwiseStateMachine bsm;
        BlockwiseStateMachine::transfer_type t;

        t.block_size(1024);
        REQUIRE(t.szx_ == 6);
        bsm.initiate_response(300, embr::coap::Option::ApplicationJson);
        bsm.encode_options(encoder);
        encoder.payload();
        encoder.finalize();
        offset = encoder.rdbuf()->pubseekoff(0, estd::ios_base::cur);

        // 4 - header
        // 1 - content format
        // 3 - size2
        // 2 - block2
        // 1 - payload marker
        REQUIRE(offset == 11);
        // DEBT: Special offset type, is there a way to do pointer-ish math directly on it?
        uint8_t c = buffer[(int)offset - 1];
        REQUIRE(c == 0xFF);
    }
}
