
#include <catch.hpp>

#include "coap_transmission.h"
#include "coap-generator.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

using namespace moducom::pipeline;
using namespace moducom::pipeline::experimental;
using namespace moducom::pipeline::layer3;
using namespace moducom::pipeline::layer3::experimental;

class TestOptionEncoderHelper : public OptionEncoderHelper
{
public:
    virtual uint16_t option_start_callback() OVERRIDE
    {
        return 0;
    }
};

TEST_CASE("CoAP message construction tests", "[coap-send]")
{
#ifndef CLEANUP_TRANSMISSION_CPP
    SECTION("1")
    {
        uint8_t expected[] =
        {
            0x60, // header v1 with ACK (piggyback response does this) and no TKL
            0x44, // response code of Changed = binary 010 00100 = 2.04
            0x00, 0x00, // message ID of 0
            0xFF, // payload marker
            'H', 'e', 'l', 'l', 'o'
        };

        Token token;

        token.length = 0;

        OutgoingResponseHandler out_handler(&token);

        out_handler.send(
                CoAP::Header::Code::Changed,
                (const uint8_t*) "Hello", 5);

        const uint8_t* result = out_handler.get_buffer();

        for(int i = 0; i < sizeof(expected); i++)
        {
            INFO("At position " << i);

            uint8_t result_ch = result[i];
            uint8_t expected_ch = expected[i];

            REQUIRE(expected_ch == result_ch);
        }
    }
#endif
    SECTION("2")
    {
        moducom::pipeline::layer3::MemoryChunk<128> chunk;
        //BufferProviderPipeline p(chunk);
        SimpleBufferedPipeline p(chunk);
        CoAPGenerator generator(p);
        // FIX: default constructor leaves header uninitialized
        // would very much prefer to avoid that if possible
        Header header;

        header.raw = 0;

        uint8_t expected_header_bytes[] = { 0x14, 0x77, 0, 1 };

        chunk.memset(0);

        header.message_id(1);
        header.token_length(4);
        header.code(0x77);
        header.type(Header::NonConfirmable);

        int i = 0;

        REQUIRE(header.bytes[i] == expected_header_bytes[i]); i++;
        REQUIRE(header.bytes[i] == expected_header_bytes[i]); i++;
        REQUIRE(header.bytes[i] == expected_header_bytes[i]); i++;
        REQUIRE(header.bytes[i] == expected_header_bytes[i]); i++;

        generator._output(header);

        CONSTEXPR int option_number = 4;
        CONSTEXPR int option_length = 4;

        moducom::coap::experimental::layer2::OptionBase option(option_number);

        option.value_string = "TEST";
        option.length = option_length;

        generator.output_option_begin(option);
        generator._output(option);

        i = 0;

        REQUIRE(chunk[i] == expected_header_bytes[i]); i++;
        REQUIRE(chunk[i] == expected_header_bytes[i]); i++;
        REQUIRE(chunk[i] == expected_header_bytes[i]); i++;
        REQUIRE(chunk[i] == expected_header_bytes[i]); i++;

        REQUIRE(chunk[i++] == ((option_number << 4) | option_length));
        REQUIRE(chunk[i++] == 'T');
        REQUIRE(chunk[i++] == 'E');
        REQUIRE(chunk[i++] == 'S');
        REQUIRE(chunk[i++] == 'T');
    }
    SECTION("OptionEncoderHelper")
    {
        layer3::MemoryChunk<128> chunk;
        SimpleBufferedPipeline output(chunk);
        CoAPGenerator encoder(output);
        TestOptionEncoderHelper oeh;

        oeh.initialize(encoder);
        // FIX: VC++ sticks in an infinite loop here
        while(!oeh.process_iterate(encoder))
        {

        }
    }
}
