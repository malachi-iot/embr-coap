#define CLEANUP

#include <catch.hpp>

#include "../coap.h"
#include "../coap_transmission.h"
#include "../coap-decoder.h"
#include "test-data.h"

using namespace moducom::coap;

TEST_CASE("CoAP low level tests", "[coap-lowlevel]")
{
    SECTION("Basic header construction")
    {
        CoAP::Header header(CoAP::Header::Confirmable);

        REQUIRE(header.bytes[0] == 0x40);
        REQUIRE(header.bytes[1] == 0);
        REQUIRE(header.bytes[2] == 0);
        REQUIRE(header.bytes[3] == 0);

        header.type(CoAP::Header::NonConfirmable);

        REQUIRE(header.bytes[0] == 0x50);
        REQUIRE(header.bytes[1] == 0);
        REQUIRE(header.bytes[2] == 0);
        REQUIRE(header.bytes[3] == 0);
    }
    SECTION("Basic parsing")
    {
#ifdef CLEANUP
        typedef moducom::coap::Decoder parser_t;
#else
        typedef CoAP::ParserDeprecated parser_t;
#endif

        uint8_t buffer[] = {
                0x40, 0x00, 0x00, 0x00, // fully blank header
                0x11, // option with delta 1 length 1
                //0x02, // delta single byte of data
                0x03, // value single byte of data
                0x12, // option with delta 1 length 2
                0x04, // value byte of data #1
                0x05 // value byte of data #2
        };

        parser_t parser;

        for (int i = 0; i < sizeof(buffer); i++)
        {
#ifdef CLEANUP
            // A little clunky but should work, just to stay 1:1 with old test
            moducom::pipeline::MemoryChunk temp_chunk(&buffer[i], 1);
            parser.process(temp_chunk, i == sizeof(buffer) - 1);
#else
            parser.process(buffer[i]);
#endif

            switch (i + 1)
            {
                case 4:
                    REQUIRE(parser.state() == parser_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(parser.state() == parser_t::Options);
#ifdef CLEANUP
                    REQUIRE(parser.option_state() == OptionDecoder::OptionDeltaAndLengthDone);
                    REQUIRE(parser.optionHolder.number_delta == 1);
                    REQUIRE(parser.optionHolder.length == 1);
#else
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 1);
#endif
                    break;

                case 6:
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(parser.state() == parser_t::Options);
#ifdef CLEANUP
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValueDone);
#else
                    REQUIRE(parser.sub_state() == parser_t::OptionValueDone);
#endif
                    break;

                case 7:
                    REQUIRE(parser.state() == parser_t::Options);
#ifdef CLEANUP
                    REQUIRE(parser.optionHolder.number_delta == 2); // Our delta trick auto adds, thus the divergence in test from below
                    REQUIRE(parser.optionHolder.length == 2);
#else
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 2);
#endif
                    break;

                case 8:
                    REQUIRE(parser.state() == parser_t::Options);
#ifdef CLEANUP
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValue);
#else
                    REQUIRE(parser.sub_state() == parser_t::OptionValue);
#endif
                    break;

                case 9:
#ifdef CLEANUP
                    // an important divergence from previous Parser, we arrive at OptionsDone earlier since
                    // it has more knowledge onhand than Parser did, and therefore can figure that state out
                    REQUIRE(parser.state() == parser_t::OptionsDone);
                    REQUIRE(parser.option_state() == OptionDecoder::OptionValueDone);
#else
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionValueDone);
#endif
                    break;
            }
        }
    }
    SECTION("Basic Parsing: 16 bit")
    {
        typedef CoAP::ParserDeprecated parser_t;

        uint8_t* buffer = buffer_16bit_delta;

        parser_t parser;

        for (int i = 0; i < sizeof(buffer_16bit_delta); i++)
        {
            uint8_t value = buffer[i];

            parser.process(value);

            parser_t::State state = parser.state();
            parser_t::SubState sub_state = parser.sub_state();

            uint16_t option_delta = parser.option_delta();
            uint16_t option_length = parser.option_length();

            switch (i + 1)
            {
                case 4:
                    REQUIRE(state == parser_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(state == parser_t::Options);
                    // TODO: Fix clumsiness of state inspection here with "non processed" mode
                    // where process returns false
                    REQUIRE(sub_state == parser_t::OptionDelta);
                    REQUIRE(option_length == 1);
                    break;

                case 6:
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionDelta);
                    break;

                case 7:
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionDeltaDone);
                    REQUIRE(option_delta == 270);
                    break;

                case 8:
                    // Because option length was handled during OptionSize (not extended)
                    // then we jump right to OptionValue
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionValueDone);
                    break;

                case 9:
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(option_delta == 1);
                    REQUIRE(option_length == 2);
                    break;

                case 10:
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionValue);
                    break;

                case 11:
                    REQUIRE(state == parser_t::Options);
                    REQUIRE(sub_state == parser_t::OptionValueDone);
                    break;
            }
        }
    }
}