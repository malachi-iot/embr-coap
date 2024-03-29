#define CLEANUP

#include <catch2/catch.hpp>
#include <embr/observer.h>

#include "coap_transmission.h"
#include "coap/decoder.h"
#include "test-data.h"

using namespace embr::coap;

TEST_CASE("CoAP low level tests", "[coap-lowlevel]")
{
    SECTION("Basic parsing")
    {
        uint8_t buffer[] = {
                0x40, 0x00, 0x00, 0x00, // 1-4: fully blank header
                0x11, // 5: option with delta 1 length 1
                //0x02, // delta single byte of data
                0x03, // 6: value single byte of data
                0x12, // 7: option with delta 1 length 2
                0x04, // 8: value byte of data #1
                0x05  // 9: value byte of data #2
        };

        Decoder parser;
        typedef internal::_root_state_t _state_t;

        for (int i = 0; i < sizeof(buffer); i++)
        {
            internal::const_buffer temp_chunk(&buffer[i], 1);

            embr::void_subject s;
            Decoder::Context context(temp_chunk, i == sizeof(buffer) - 1);

            // Low level test, we're evaluating inching through tiny buffers so
            // we manually need to look for eof/waitstate
            iterated::decode_result r;

            do
            {
                r = iterated::decode_and_notify(parser, s, context);
            }
            while(!(r.eof | r.waitstate));

            switch (i + 1)
            {
                case 4:
                    REQUIRE(parser.state() == _state_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(parser.state() == _state_t::Options);
                    REQUIRE(parser.option_state() == OptionDecoder::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_number() == 1);
                    REQUIRE(parser.option_length() == 1);
                    break;

                case 6:
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(parser.state() == _state_t::Options);
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValueDone);
                    break;

                case 7:
                    REQUIRE(parser.state() == _state_t::Options);
                    REQUIRE(parser.option_number() == 2); // Our delta trick auto adds, thus the divergence in test from below
                    REQUIRE(parser.option_length() == 2);
                    break;

                case 8:
                    REQUIRE(parser.state() == _state_t::Options);
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValue);
                    break;

                case 9:
                    // an important divergence from previous Parser, we arrive at OptionsDone earlier since
                    // it has more knowledge onhand than Parser did, and therefore can figure that state out
                    REQUIRE(parser.state() == _state_t::OptionsDone);
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValueDone);
                    break;
            }
        }
    }
    SECTION("Basic Parsing: 16 bit")
    {
        typedef Decoder parser_t;
        typedef OptionDecoder option_parser_t;
        typedef internal::_root_state_t _state_t;
        typedef internal::root_state_t state_t;

        const uint8_t* buffer = buffer_16bit_delta;

        parser_t parser;

        for (int i = 0; i < sizeof(buffer_16bit_delta); i++)
        {
            // A little clunky but should work, just to stay 1:1 with old test
            internal::const_buffer temp_chunk(&buffer[i], 1);
            parser_t::Context context(temp_chunk, i == sizeof(buffer_16bit_delta) - 1);

            iterated::decode_result r;

            do
            {
                r = parser.process_iterate(context);
            }
            while(!(r.eof | r.waitstate));

            state_t state = parser.state();

            OptionDecoder::State sub_state = parser.option_state();

            uint16_t option_delta = parser.option_number();
            uint16_t option_length = parser.option_length();

            switch (i + 1)
            {
                case 4:
                    REQUIRE(state == _state_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(state == _state_t::Options);
                    // TODO: Fix clumsiness of state inspection here with "non processed" mode
                    // where process returns false
                    REQUIRE(sub_state == option_parser_t::OptionLengthDone);
                    REQUIRE(option_length == 1);
                    break;

                case 6:
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionDelta);
                    break;

                case 7:
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionDeltaDone);
                    REQUIRE(option_delta == 270);
                    break;

                case 8:
                    // Because option length was handled during FirstByte (not extended)
                    // then we jump right to OptionValue
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionValueDone);
                    break;

                case 9:
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(option_delta == 271);
                    REQUIRE(option_length == 2);
                    break;

                case 10:
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionValue);
                    break;

                case 11:
                    REQUIRE(state == _state_t::Options);
                    REQUIRE(sub_state == option_parser_t::OptionValueDone);
                    break;

                case 12:
                    REQUIRE(state == _state_t::OptionsDone);
                    break;
            }
        }
    }
}
