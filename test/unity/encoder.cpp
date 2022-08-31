// TODO: NetBuf encoder still uses mk. 1 netbuf which we don't want to invest
// time into, so revise that so that we can code a test here
#include "unit-test.h"

#include <estd/ostream.h>
#include <estd/span.h>

#include <coap/encoder/streambuf.h>

#include "unit-test.h"

using namespace embr;

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

static void test_build_reply()
{
    char buffer[512];
    estd::span<char> buf(&buffer[0], 512);
    encoder_type encoder(buf);
    coap::TokenAndHeaderContext<true> context;

    // represents synthetic incoming header
    coap::Header h(coap::Header::Confirmable);

    h.message_id(0x123);

    context.header(h);

    coap::build_reply(context, encoder, 200);

    coap::Header h2;

    estd::copy_n(buffer, 4, h2.bytes);

    TEST_ASSERT_EQUAL(1, h2.version());
    TEST_ASSERT_EQUAL(0, h2.token_length());
    TEST_ASSERT_EQUAL(200, h2.code());
    TEST_ASSERT_EQUAL(0x123, h2.message_id());

    // 'Accept' option actually doesn't belong in replies, but we'll allow it for
    // synthetic.  Also, since 'Accept' is 17, it hangs over the 13-boundary and
    // results in a two-byte option header
    encoder.option_int(coap::Option::Accept, coap::Option::ApplicationJson);

    // add in payload marker
    encoder.payload();

    // FIX: Not able to resolve underlying dependency here
    //TEST_ASSERT_EQUAL(4, encoder.rdbuf()->pubseekpos(0));

    TEST_ASSERT_EQUAL(8, encoder.rdbuf()->pos());
}

static void test_build_reply_stream()
{
    char buffer[512];
    estd::span<char> buf(&buffer[0], 512);
    encoder_type encoder(buf);
    coap::TokenAndHeaderContext<true> context;

    // represents synthetic incoming header
    coap::Header h(coap::Header::Confirmable);

    h.message_id(0x123);

    context.header(h);

    coap::build_reply(context, encoder, coap::Header::Code::Valid);

    encoder.payload();

    encoder_type::ostream_type out = encoder.ostream();

    out << '{';
    out << "hi2u";
    out << '}';

    encoder.finalize();

    TEST_ASSERT_EQUAL(11, encoder.rdbuf()->pos());
}

#ifdef ESP_IDF_TESTING
TEST_CASE("encoder tests", "[encoder]")
#else
void test_encoder()
#endif
{
    RUN_TEST(test_encoder_1);
    RUN_TEST(test_build_reply);
    RUN_TEST(test_build_reply_stream);
}