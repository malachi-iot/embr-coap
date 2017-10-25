
#include <catch.hpp>

#include "../coap.h"

using namespace moducom::coap;

TEST_CASE("CoAP tests", "[coap]")
{
    SECTION("Basic header construction")
    {
        CoAP::Header header(CoAP::Header::Confirmable);

        REQUIRE(header.raw == 0x40000000);

        header.type(CoAP::Header::NonConfirmable);

        REQUIRE(header.raw == 0x50000000);
    }
    SECTION("Basic parsing")
    {
        typedef CoAP::Parser parser_t;

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
            parser.process(buffer[i]);

            switch (i + 1)
            {
                case 4:
                    REQUIRE(parser.get_state() == parser_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(parser.get_state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 1);
                    break;

                case 6:
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(parser.get_state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionValueDone);
                    break;

                case 7:
                    REQUIRE(parser.get_state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 2);
                    break;
            }
        }
    }
}
