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
    typedef CoAP::OptionExperimental number_t;

    SECTION("1")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter writer(chunk);
        moducom::coap::experimental::BlockingEncoder encoder(writer);

        encoder.header(CoAP::Header::Get);

        encoder.option(number_t::ETag, "etag");
        encoder.option(number_t::UriPath, "query");

        encoder.payload("A payload");

        const int option_pos = 4;

        REQUIRE(chunk[option_pos + 0] == 0x44);
        REQUIRE(chunk[option_pos + 5] == 0x75);
        REQUIRE(chunk[option_pos + 6] == 'q');
    }
    SECTION("Token test")
    {
        moducom::coap::layer2::Token token;

        token.set(0);

        token[0] = '3';

        REQUIRE(token[0] == '3');
    }
}