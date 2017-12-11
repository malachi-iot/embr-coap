#include <catch.hpp>

#include "../coap-decoder.h"
//#include "../mc/pipeline.h"

using namespace moducom::coap;
using namespace moducom::pipeline;

static uint8_t buffer_16bit_delta[] = {
        0x40, 0x00, 0x00, 0x00, // 4: fully blank header
        0xE1, // 5: option with delta 1 length 1
        0x00, // 6: delta ext byte #1
        0x01, // 7: delta ext byte #2
        0x03, // 8: value single byte of data
        0x12, // 9: option with delta 1 length 2
        0x04, //10: value byte of data #1
        0x05, //11: value byte of data #2
        0xFF, //12: denotes a payload presence
        // payload itself
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
};

TEST_CASE("CoAP decoder tests", "[coap-decoder]")
{
    //layer3::MemoryChunk<1024> buffer_in;
    //buffer_in.memcpy(buffer_16bit_delta, sizeof(buffer_16bit_delta));

    MemoryChunk buffer_in(buffer_16bit_delta, sizeof(buffer_16bit_delta));


    layer3::SimpleBufferedPipeline net_in(buffer_in);

    SECTION("Basic test 1")
    {
        OptionDecoder decoder;
        OptionDecoder::OptionExperimental option;

        // pretty much ready to TRY testing, just need to load in appropriate data into buffer_in
        //decoder.process_iterate(net_in, &option);
    }
}