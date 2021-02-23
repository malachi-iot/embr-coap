// TODO: NetBuf encoder still uses mk. 1 netbuf which we don't want to invest
// time into, so revise that so that we can code a test here
#include "unit-test.h"

#include <estd/ostream.h>
#include <estd/span.h>

#include <coap/encoder/streambuf.h>

#include "unit-test.h"

using namespace moducom;

typedef estd::experimental::ospanbuf streambuf_type;
typedef coap::StreambufEncoder<streambuf_type> encoder_type;

static void test_encoder_1()
{
    char buffer[512];
    estd::span<char> buf(&buffer[0], 512);
    encoder_type encoder(buf);
    // DEBT: A little confusing, Header needs a parameter before it initializes version field
    coap::Header h(coap::Header::Confirmable);

    //h.message_id(000);

    encoder.header(h);

    encoder.option(coap::Option::UriPath, 4);

    encoder_type::ostream_type out = encoder.ostream();

    out << "TEST";

    // FIX: Having an issue probably
    encoder.option(coap::Option::UriPath, 3);

    out << "POS";

    encoder.payload();

    out.put(0x10);
    out.put(0x11);
    out.put(0x12);
    out.put(0x13);
    out.put(0x14);
    out.put(0x15);
    out.put(0x16);

    encoder.finalize();

    int sz = out.tellp();

    TEST_ASSERT_EQUAL(sizeof(buffer_plausible), sz);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&buffer_plausible[0], out.rdbuf()->pbase(), sz);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("encoder tests", "[encoder]")
#else
void test_encoder()
#endif
{
    RUN_TEST(test_encoder_1);
}