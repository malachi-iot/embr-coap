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
    coap::StreambufDecoder<streambuf_type> decoder(in);

    bool eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::Header, decoder.state());

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::HeaderDone, decoder.state());

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::TokenDone, decoder.state());

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::OptionsStart, decoder.state());

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    // DEBT: Don't like repeatedly kicking top-level CoAP parser to do clicky decode of
    // option.  Would be better to know to use option decoder and otherwise fast-forward option.
    // Both are non-obvious, but at least the latter is more organized
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());

    TEST_ASSERT_EQUAL(1, decoder.option_length());
    TEST_ASSERT_EQUAL(0, decoder.option_number());

    TEST_ASSERT_EQUAL(coap::OptionDecoder::OptionLengthDone, decoder.option_decoder().state());
    //decoder.option();
    //TEST_ASSERT_EQUAL(3, decoder.option()[0]);

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());
    TEST_ASSERT_EQUAL(coap::OptionDecoder::ValueStart, decoder.option_decoder().state());

    TEST_ASSERT_EQUAL(270, decoder.option_number());
    TEST_ASSERT_EQUAL(3, decoder.option()[0]);

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    //TEST_ASSERT_EQUAL(coap::Decoder::OptionsDone, decoder.state());
}

#ifdef ESP_IDF_TESTING
TEST_CASE("decoder tests", "[decoder]")
#else
void test_decoder()
#endif
{
    RUN_TEST(test_basic_decode);
    RUN_TEST(test_streambuf_decode);
}