
#include <catch.hpp>

#include "../coap.h"
#include "../coap_transmission.h"

using namespace moducom::coap;

static uint8_t buffer_16bit_delta[] = {
    0x40, 0x00, 0x00, 0x00, // 4: fully blank header
    0xE1, // 5: option with delta 1 length 1
    0x00, // 6: delta ext byte #1
    0x01, // 7: delta ext byte #2
    0x03, // 8: value single byte of data
    0x12, // 9: option with delta 1 length 2
    0x04, //10: value byte of data #1
    0x05, //11: value byte of data #2
    0xFF, //12: denotes a payload presence
    // payload itself
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};



class TestResponder : public CoAP::IResponder
{
public:
    virtual void OnHeader(const CoAP::Header header);
    virtual void OnOption(const CoAP::OptionExperimental& option);
    virtual void OnPayload(const uint8_t message[], size_t length);
};


void TestResponder::OnHeader(const CoAP::Header header) 
{
}

void TestResponder::OnOption(const CoAP::OptionExperimental& option)
{
    switch (option.number)
    {
        case 270:
            REQUIRE(option.length == 1);
            break;

        case 271:
            REQUIRE(option.length == 2);
            break;

        default:
            FAIL("Unexpected Option Number: " << option.number);
    }
}

void TestResponder::OnPayload(const uint8_t message[], size_t length)
{
    REQUIRE(length == 7);
    REQUIRE(memcmp(&buffer_16bit_delta[12], message, length) == 0);
}


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
                    REQUIRE(parser.state() == parser_t::HeaderDone);
                    break;

                case 5:
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 1);
                    break;

                case 6:
                    // Because it's only one byte, we don't get to see OptionValue since it's 1:1 with
                    // OptionLengthDone/OptionDeltaAndLengthDone
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionValueDone);
                    break;

                case 7:
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionDeltaAndLengthDone);
                    REQUIRE(parser.option_delta() == 1);
                    REQUIRE(parser.option_length() == 2);
                    break;

                case 8:
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionValue);
                    break;

                case 9:
                    REQUIRE(parser.state() == parser_t::Options);
                    REQUIRE(parser.sub_state() == parser_t::OptionValueDone);
                    break;
            }
        }
    }
    SECTION("Basic Parsing: 16 bit")
    {
        typedef CoAP::Parser parser_t;

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
    SECTION("Parsing to IResponder")
    {
        typedef CoAP::Parser parser_t;

        TestResponder responder;

        CoAP::ParseToIResponder p(&responder);

        p.process(buffer_16bit_delta, sizeof(buffer_16bit_delta));

    }
    SECTION("Basic generating single option")
    {
        uint8_t buffer[64];
        int counter = 0;

        experimental::layer2::OptionBase o(1);

        o.length = 1;
        o.value_string = "a";

        experimental::layer2::OptionGenerator::StateMachine sm(o);

        while(sm.sub_state() != CoAP::Parser::OptionValueDone)
        {
            buffer[counter++] = sm.generate();
        }

        REQUIRE(buffer[0] == 0x11);
        REQUIRE(buffer[1] == 'a');
        REQUIRE(counter == 2);
    }
    SECTION("Basic generating single option: 16-bit")
    {
        uint8_t buffer[64];
        int counter = 0;

        experimental::layer2::OptionBase o(10000);

        o.length = 1;
        o.value_string = "a";

        experimental::layer2::OptionGenerator::StateMachine sm(o);

        while(sm.sub_state() != CoAP::Parser::OptionValueDone)
        {
            buffer[counter++] = sm.generate();
        }

        REQUIRE(buffer[0] == 0xE1);
        REQUIRE(buffer[1] == ((10000 - 269) >> 8));
        REQUIRE(buffer[2] == ((10000 - 269) & 0xFF));
        REQUIRE(buffer[3] == 'a');
        REQUIRE(counter == 4);
    }
}
