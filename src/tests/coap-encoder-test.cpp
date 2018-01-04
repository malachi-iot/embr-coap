#include <catch.hpp>
#include "../coap-encoder.h"
#include "../mc/memory.h"
#include "../coap_transmission.h"
#include "../coap-token.h"
#include "../mc/experimental.h"

using namespace moducom::coap;
using namespace moducom::pipeline;




TEST_CASE("CoAP encoder tests", "[coap-encoder]")
{
    typedef Option number_t;

    SECTION("1")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter writer(chunk);
        moducom::coap::experimental::BlockingEncoder encoder(writer);

        encoder.header(Header::Get);

        encoder.option(number_t::ETag, "etag");
        encoder.option(number_t::UriPath, "query");

        encoder.payload("A payload");

        const int option_pos = 4;

        REQUIRE(chunk[option_pos + 0] == 0x44);
        REQUIRE(chunk[option_pos + 5] == 0x75);
        REQUIRE(chunk[option_pos + 6] == 'q');
    }
    SECTION("Experimental encoder")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter writer(chunk);
        moducom::coap::experimental::BufferedEncoder encoder(writer);
        Header& header = *encoder.header();
        moducom::coap::layer1::Token& token = *encoder.token();

        header.token_length(2);
        header.type(Header::Confirmable);
        header.code(Header::Get);
        header.message_id(0xAA);

        token[0] = 1;
        token[1] = 2;

        encoder.option(number_t::UriPath,
                       moducom::pipeline::MemoryChunk((uint8_t*)"Test", 4));

        moducom::pipeline::MemoryChunk chunk2 = encoder.payload();

        int l = sprintf((char*)chunk2.__data(), "Guess we'll do it directly %s", "won't we");

        // remove null terminator
        encoder.advance(l - 1);

        const int option_pos = 5;

        REQUIRE(chunk[4] == 1);
        REQUIRE(chunk[5] == 2);
        REQUIRE(chunk[option_pos + 6] == 0xFF);
    }
    SECTION("Token test")
    {
        moducom::coap::layer2::Token token;

        token.set(0);

        token[0] = '3';

        REQUIRE(token[0] == '3');
    }
}