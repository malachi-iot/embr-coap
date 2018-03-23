#include <catch.hpp>

#include "../coap-decoder.h"
#include "test-data.h"
//#include "../mc/pipeline.h"

using namespace moducom::coap;
using namespace moducom::pipeline;

TEST_CASE("CoAP decoder tests", "[coap-decoder]")
{
    //layer3::MemoryChunk<1024> buffer_in;
    //buffer_in.memcpy(buffer_16bit_delta, sizeof(buffer_16bit_delta));

    MemoryChunk::readonly_t buffer_in(buffer_16bit_delta);


    layer3::SimpleBufferedPipeline net_in(buffer_in);

    SECTION("Basic test 1")
    {
        OptionDecoder decoder;
        OptionDecoder::OptionExperimental option;

        // pretty much ready to TRY testing, just need to load in appropriate data into buffer_in
        //decoder.process_iterate(net_in, &option);
    }
    SECTION("Payload only test")
    {
        Decoder decoder;
        MemoryChunk::readonly_t chunk(buffer_payload_only);

        // FIX: not quite handling payload-only correctly yet
        decoder.process(chunk);
    }
}