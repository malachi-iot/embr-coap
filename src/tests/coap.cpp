
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


static uint8_t buffer_plausible[] = {
    0x40, 0x00, 0x00, 0x00, // 4: fully blank header
    0xB4, // 5: option delta 11 (Uri-Path) with value length of 4
    'T',
    'E',
    'S',
    'T',
    0x03, // 9: option delta 0 (Uri-Path, again) with value length of 3
    'P', 'O', 'S',
    0xFF,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};


static void empty_fn()
{

}


class TestResponder : public CoAP::IResponderDeprecated
{
public:
    virtual void OnHeader(const CoAP::Header header);
    virtual void OnToken(const uint8_t message[], size_t length) OVERRIDE;
    virtual void OnOption(const CoAP::OptionExperimentalDeprecated& option);
    virtual void OnPayload(const uint8_t message[], size_t length);
};


void TestResponder::OnHeader(const CoAP::Header header) 
{
}


void TestResponder::OnToken(const uint8_t *message, size_t length)
{

}

void TestResponder::OnOption(const CoAP::OptionExperimentalDeprecated& option)
{
    switch (option.number)
    {
        case 270:
            REQUIRE(option.length == 1);
            break;

        case 271:
            REQUIRE(option.length == 2);
            break;

        case CoAP::OptionExperimentalDeprecated::UriPath:
            // FIX: First failure is option.length coming in
            // at 3, which suggests the TEST portion is not
            // coming through properly (have a feeling it's
            // getting eaten by our uri matcher)
            REQUIRE(option.length == 4);
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
    SECTION("Parsing to IResponderDeprecated")
    {
        typedef CoAP::ParserDeprecated parser_t;

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

        while(sm.state() != CoAP::ParserDeprecated::OptionValueDone)
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

        while(sm.state() != CoAP::ParserDeprecated::OptionValueDone)
        {
            buffer[counter++] = sm.generate();
        }

        REQUIRE(buffer[0] == 0xE1);
        REQUIRE(buffer[1] == ((10000 - 269) >> 8));
        REQUIRE(buffer[2] == ((10000 - 269) & 0xFF));
        REQUIRE(buffer[3] == 'a');
        REQUIRE(counter == 4);
    }
    SECTION("Parsing thru experimental DispatchingResponder")
    {
        moducom::coap::experimental::DispatchingResponder responder;
        TestResponder user_responder;

        responder.add_handle("test", &user_responder);
        responder.add_handle("TEST/POS/", &user_responder);

        typedef CoAP::ParserDeprecated parser_t;

        CoAP::ParseToIResponder p(&responder);

        p.process(buffer_plausible, sizeof(buffer_plausible));
    }
    SECTION("Experimental CoAP::Header::Code tests")
    {
        CoAP::Header header;

        header.raw = 0;
        header.response_code(CoAP::Header::Code::ProxyingNotSupported);

        CoAP::Header::Code& code = header.code_experimental();

        REQUIRE(code.get_class() == 5);
        REQUIRE(code.detail() == 05);
    }
}
