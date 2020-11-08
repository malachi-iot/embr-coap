// uses too many C++11 goodies at the moment
//#include <coap/decoder/netbuf.h>
#include <coap/decoder.h>

#include "unit-test.h"

using namespace moducom;

static void test_basic_decode()
{
    estd::span<const uint8_t> in(buffer_16bit_delta, sizeof(buffer_16bit_delta));
    coap::Decoder decoder;
    coap::Decoder::Context context(in, true);

    // This one's a bit tricky because VisualDSP won't let a 2nd "decoder.cpp"
    // into the project, even at different folder levels
    //decoder.process_iterate(context);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("decoder tests", "[decoder]")
#else
void test_decoder()
#endif
{
    test_basic_decode();
}