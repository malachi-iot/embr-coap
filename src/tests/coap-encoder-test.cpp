#include <catch.hpp>
#include "../coap-encoder.h"
#include "mc/memory.h"
#include "../coap_transmission.h"
#include "../coap-token.h"
#include "../mc/experimental.h"
#include "coap/encoder.hpp"

#include "exp/netbuf.h"

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

        int l = sprintf((char*)chunk2.data(), "Guess we'll do it directly %s.", "won't we");

        // remove null terminator
        encoder.advance(l - 1);
        //encoder.payload("Now for something completely different");

        encoder.complete();

        const int option_pos = 5;

//#if !defined (__APPLE__)
        // FIX: on MBP QTcreator this fails
        REQUIRE(chunk[4] == 1);
        REQUIRE(chunk[5] == 2);
        REQUIRE(chunk[option_pos + 6] == 0xFF);
//#endif
    }
    // TODO: Move this to more proper test file location
    SECTION("Read Only Memory Buffer")
    {
        // TODO: Make a layer2::string merely to be a wrapper around const char*
        MemoryChunk::readonly_t str = MemoryChunk::readonly_t::str_ptr("Test");

        REQUIRE(str.length() == 4);
    }
    SECTION("NetBuf encoder")
    {
        typedef moducom::io::experimental::layer2::NetBufMemoryWriter<256> netbuf_t;
        netbuf_t netbuf;
        // FIX: netbuf.chunk() broken in this context in that the data
        // it's inspecting appears to not be the netbuf_t buffer.  length is correct
        MemoryChunk chunk = netbuf.chunk();
        //const moducom::pipeline::layer1::MemoryChunk<256>& chunk = netbuf.chunk();
        const uint8_t* data = netbuf.data();

        moducom::coap::layer2::Token token;

        token[0] = 1;
        token[1] = 2;
        token[2] = 3;
        token.length(3);

        Header header;

        header.token_length(token.length());

        NetBufEncoder<netbuf_t&> encoder(netbuf);
        Option::Numbers n = Option::UriPath;

        encoder.header(header);

        // Header is always 4
        size_t expected_msg_size = 4;

        REQUIRE(netbuf.length() + expected_msg_size == chunk.length());

        encoder.token(token);

        REQUIRE(data[expected_msg_size] == 1);
        REQUIRE(data[expected_msg_size + 1] == 2);

        expected_msg_size += token.length();

        REQUIRE(netbuf.length() + expected_msg_size == chunk.length());

        encoder.option(n, MemoryChunk((uint8_t*)"test", 4));

        REQUIRE(0xB4 == data[expected_msg_size]); // option 11, length 4

        netbuf_t& w = encoder.netbuf();

        // + option "header" of size 1 + option_value of 4
        expected_msg_size += 1 + 4;

        // TODO: Make a distinctive netbuf length_written vs length_free, right now
        // length() represents amount available in current 'PBUF'
        REQUIRE(netbuf.length() + expected_msg_size == chunk.length());
        //REQUIRE(0xB4 == chunk.data()[4]); // option 11, length 4

        encoder.option(n, std::string("test2"));

        REQUIRE(data[expected_msg_size] == 0x05); // still option 11, length 5

        // option "header" of size 1 + option_value of size 5
        expected_msg_size += (1 + 5);

        REQUIRE(netbuf.length() + expected_msg_size == chunk.length());

        encoder.payload(std::string("payload"));

        REQUIRE(data[expected_msg_size] == 0xFF); // payload marker
        REQUIRE(data[expected_msg_size + 1] == 'p');

        expected_msg_size += (1 + 7); // payload marker + "payload"

        REQUIRE(netbuf.length() + expected_msg_size == chunk.length());
    }
}
