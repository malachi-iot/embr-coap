// uses too many C++11 goodies at the moment
//#include <coap/decoder/netbuf.h>
#include <coap/decoder.h>
#include <coap/decoder/streambuf.hpp>

#include "unit-test.h"

using namespace moducom;

static estd::span<const uint8_t> in(buffer_16bit_delta, sizeof(buffer_16bit_delta));

static void test_basic_decode()
{
    coap::Decoder decoder;
    coap::Decoder::Context context(in, true);

    // This one's a bit tricky because VisualDSP won't let a 2nd "decoder.cpp"
    // into the project, even at different folder levels
    //decoder.process_iterate(context);
}


static void test_streambuf_decode()
{
    typedef estd::experimental::ispanbuf streambuf_type;
    // DEBT: Quite clumsy streambuf char requirement vs coap uint8_t requirement.
    estd::span<char> in((char*)buffer_16bit_delta, sizeof(buffer_16bit_delta));
    streambuf_type sb(in);
    //coap::StreambufDecoder<streambuf_type> decoder(sb);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("decoder tests", "[decoder]")
#else
void test_decoder()
#endif
{
    test_basic_decode();
}