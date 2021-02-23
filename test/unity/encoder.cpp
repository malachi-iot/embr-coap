// TODO: NetBuf encoder still uses mk. 1 netbuf which we don't want to invest
// time into, so revise that so that we can code a test here
#include "unit-test.h"

#include <estd/ostream.h>
#include <estd/span.h>

#include <coap/encoder/streambuf.h>

using namespace moducom;

typedef estd::experimental::ospanbuf streambuf_type;
typedef coap::StreambufEncoder<streambuf_type> encoder_type;

static void test_encoder_1()
{
    char buffer[512];
    estd::span<char> buf(&buffer[0], 512);
    encoder_type encoder(buf);
    coap::Header h;

    h.type(coap::Header::Acknowledgement);

    encoder_type::ostream_type out = encoder.ostream();

    encoder.header(h);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("encoder tests", "[encoder]")
#else
void test_encoder()
#endif
{
    RUN_TEST(test_encoder_1);
}