#include <coap/decoder.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/decoder/subject-core.hpp>

#include "unit-test.h"

using namespace embr;

static estd::span<const uint8_t> in(buffer_16bit_delta, sizeof(buffer_16bit_delta));
typedef estd::experimental::ispanbuf streambuf_type;

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
    // DEBT: Quite clumsy streambuf char requirement vs coap uint8_t requirement.
    estd::span<char> in((char*)buffer_16bit_delta, sizeof(buffer_16bit_delta));
    coap::StreambufDecoder<streambuf_type> decoder(in);

    bool eol = decoder.process_iterate_streambuf().eof;

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::Header, decoder.state());

    eol = decoder.process_iterate_streambuf().eof;

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::HeaderDone, decoder.state());

    eol = decoder.process_iterate_streambuf().eof;

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::TokenDone, decoder.state());

    eol = decoder.process_iterate_streambuf().eof;

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::OptionsStart, decoder.state());

    eol = decoder.process_iterate_streambuf().eof;

    TEST_ASSERT(!eol);
    // DEBT: Don't like repeatedly kicking top-level CoAP parser to do clicky decode of
    // option.  Would be better to know to use option decoder and otherwise fast-forward option.
    // Both are non-obvious, but at least the latter is more organized
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());

    TEST_ASSERT_EQUAL(1, decoder.option_length());

#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
    TEST_ASSERT_EQUAL(0, decoder.option_number());

    TEST_ASSERT_EQUAL(coap::OptionDecoder::OptionLengthDone, decoder.option_decoder().state());
    //decoder.option();
    //TEST_ASSERT_EQUAL(3, decoder.option()[0]);

    eol = decoder.process_iterate_streambuf();

    TEST_ASSERT(!eol);
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());
#endif
    TEST_ASSERT_EQUAL(coap::OptionDecoder::ValueStart, decoder.option_decoder().state());

    TEST_ASSERT_EQUAL(270, decoder.option_number());
    TEST_ASSERT_EQUAL(3, decoder.option()[0]);

    TEST_ASSERT(!decoder.process_iterate_streambuf().eof);
    TEST_ASSERT_EQUAL(coap::OptionDecoder::OptionValueDone, decoder.option_decoder().state());

    TEST_ASSERT(!decoder.process_iterate_streambuf().eof);
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());
    TEST_ASSERT_EQUAL(coap::OptionDecoder::ValueStart, decoder.option_decoder().state());
    TEST_ASSERT_EQUAL(271, decoder.option_number());

    TEST_ASSERT(!decoder.process_iterate_streambuf().eof);
    TEST_ASSERT_EQUAL(coap::Decoder::Options, decoder.state());
    TEST_ASSERT_EQUAL(coap::OptionDecoder::OptionValueDone, decoder.option_decoder().state());

    TEST_ASSERT(!decoder.process_iterate_streambuf().eof);
    TEST_ASSERT_EQUAL(coap::Decoder::OptionsDone, decoder.state());

    TEST_ASSERT(!decoder.process_iterate_streambuf().eof);
    TEST_ASSERT_EQUAL(coap::Decoder::Payload, decoder.state());
}

struct Context : coap::TokenAndHeaderContext<true>
{
    int counter;
};


// DEBT: Since Listener is an observer, not a subject, we must
// implement all possible notifications here with context
struct Listener
{
    void notify(coap::event::header e, Context& context)
    {
        context.header(e.h);
        context.counter++;
    }

    void notify(coap::event::token e, Context& context)
    {
        context.token(e.chunk);
        context.counter++;
    }

    void notify(coap::event::option_start, Context&) {}
    void notify(coap::event::option_completed, Context&) {}
    void notify(coap::event::internal::no_payload, Context&) {}

    void notify(coap::event::option e, Context& context)
    {
        if(context.counter++ == 2)
            TEST_ASSERT_EQUAL(270, e.option_number);
        else
            TEST_ASSERT_EQUAL(271, e.option_number);
    }

    void notify(coap::event::streambuf_payload<streambuf_type> e, Context& context)
    {
        context.counter++;
    }

    void notify(coap::event::completed e, Context& context)
    {
        context.counter++;
    }
};


static void test_decode_and_notify()
{
    // DEBT: Quite clumsy streambuf char requirement vs coap uint8_t requirement.
    estd::span<char> in((char*)buffer_16bit_delta, sizeof(buffer_16bit_delta));
    coap::StreambufDecoder<streambuf_type> decoder(in);
    Listener l;
    Context context;

    context.counter = 0;

    // NOTE: We could have used coap::decode_and_notify but I like the lower
    // level testing done here
    while(!coap::iterated::decode_and_notify(decoder, l, context).eof);

    // runs once more at end to trigger completed event, don't remember
    // exactly why we have to do it that way - it has something to do with
    // giving consumer chance to fully consume payload
    coap::iterated::decode_and_notify(decoder, l, context);

    TEST_ASSERT_EQUAL_INT(6, context.counter);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("decoder tests", "[decoder]")
#else
void test_decoder()
#endif
{
    RUN_TEST(test_basic_decode);
    RUN_TEST(test_streambuf_decode);
    RUN_TEST(test_decode_and_notify);
}