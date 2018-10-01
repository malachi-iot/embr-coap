#include <catch.hpp>

#include <stddef.h>

#include "coap-encoder.h"
#include "mc/memory.h"
#include "coap_transmission.h"
#include "coap-token.h"
#include "mc/experimental.h"
#include "coap/encoder.hpp"

#include "exp/netbuf.h"

#include <coap/streambuf-encoder.h>

using namespace moducom::coap;
using namespace moducom::pipeline;

#include <stdio.h>


TEST_CASE("CoAP encoder tests", "[coap-encoder]")
{
    typedef Option number_t;

#ifdef UNUSED
    SECTION("1")
    {
        moducom::pipeline::layer3::MemoryChunk<128> chunk;
        moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(chunk);
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
        moducom::pipeline::layer3::MemoryChunk<128> chunk;
        moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(chunk);
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
#endif
    // TODO: Move this to more proper test file location
    SECTION("Read Only Memory Buffer")
    {
        // TODO: Make a layer2::string merely to be a wrapper around const char*
        MemoryChunk::readonly_t str = MemoryChunk::readonly_t::str_ptr("Test");

        REQUIRE(str.length() == 4);
    }
    SECTION("NetBuf encoder")
    {
        typedef moducom::io::experimental::layer2::NetBufMemory<256> netbuf_t;

        // also works, external buf
        //netbuf_t netbuf;
        //NetBufEncoder<netbuf_t&> encoder(netbuf);

        NetBufEncoder<netbuf_t> encoder;
        netbuf_t& netbuf = encoder.netbuf();
        const uint8_t* data = netbuf.chunk().data();

        moducom::coap::layer2::Token token;

        token.resize(3);
        token[0] = 1;
        token[1] = 2;
        token[2] = 3;

        Header header;

        header.token_length(token.size());

        Option::Numbers n = Option::UriPath;

        encoder.header(header);

        // Header is always 4
        size_t expected_msg_size = 4;

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.token(token);

        REQUIRE(data[expected_msg_size] == 1);
        REQUIRE(data[expected_msg_size + 1] == 2);

        expected_msg_size += token.size();

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.option(n, MemoryChunk((uint8_t*)"test", 4));

        REQUIRE(0xB4 == data[expected_msg_size]); // option 11, length 4

        netbuf_t& w = encoder.netbuf();

        // + option "header" of size 1 + option_value of 4
        expected_msg_size += 1 + 4;

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.option(n, std::string("test2"));

        REQUIRE(data[expected_msg_size] == 0x05); // still option 11, length 5

        // option "header" of size 1 + option_value of size 5
        expected_msg_size += (1 + 5);

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.option(Option::ContentFormat, 50);

        REQUIRE(data[expected_msg_size] == 0x11);
        REQUIRE(data[expected_msg_size + 1] == 50);

        expected_msg_size += 2; // option 'header' + one byte for '50'

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.payload(std::string("payload"));

        REQUIRE(data[expected_msg_size] == COAP_PAYLOAD_MARKER); // payload marker
        REQUIRE(data[expected_msg_size + 1] == 'p');

        expected_msg_size += (1 + 7); // payload marker + "payload"

        REQUIRE(netbuf.length_processed() == expected_msg_size);

        encoder.complete();
    }
    SECTION("synthetic test 1")
    {
        typedef moducom::io::experimental::layer2::NetBufMemory<256> netbuf_t;

        NetBufEncoder<netbuf_t> encoder;
        uint8_t token[] = { 0, 1, 2, 3 };

        encoder.header(Header(Header::Confirmable, Header::Code::Content));
        encoder.token(token);

        encoder.payload_header();

        estd::layer2::const_string s = "hi2u";

        encoder << s;
        encoder.complete();

        netbuf_t& netbuf = encoder.netbuf();

        REQUIRE(netbuf.processed()[4] == token[0]);
        REQUIRE(netbuf.processed()[5] == token[1]);
        REQUIRE(netbuf.processed()[6] == token[2]);
        REQUIRE(netbuf.processed()[7] == token[3]);
        REQUIRE(netbuf.processed()[8] == 0xFF);
        REQUIRE(netbuf.processed()[9] == s[0]);
    }
    SECTION("streambuf encoder")
    {
        estd::layer2::const_string test_str = "hi2u";

        uint8_t buffer[128];
        estd::span<uint8_t> span(buffer, 128);
        Header header;

        header.type(Header::TypeEnum::NonConfirmable);
        header.message_id(0x1234);

        SECTION("Test 1")
        {
            embr::coap::experimental::StreambufEncoder encoder(span);

            encoder.header(header);
            encoder.option(Option::Numbers::UriPath, "hello");
            encoder.option(Option::Numbers::UriPath, test_str);
        }
    }
}
