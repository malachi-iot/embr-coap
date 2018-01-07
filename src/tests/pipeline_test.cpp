//
// Created by Malachi Burke on 11/12/17.
//

#include <catch.hpp>

#include "../mc/pipeline.h"
#include "../mc/pipeline-encoder.h"
#include <stdio.h>

using namespace moducom::pipeline;
using namespace moducom::pipeline::experimental;
//using namespace moducom::pipeline::layer3;
using namespace moducom::pipeline::layer3::experimental;

class BasicTestEncoder
{
    uint8_t counter;

public:
    BasicTestEncoder() : counter(0) {}

    typedef int16_t output_t;

    static CONSTEXPR output_t signal_continue = -1;

    output_t process_iterate()
    {
        if(counter++ == 10)
        {
            counter = 0;
            return signal_continue;
        }

        return counter;
    }
};

TEST_CASE("Pipeline basic tests", "[pipeline]")
{
    SECTION("1")
    {
        MemoryChunk chunk;
        layer3::SimpleBufferedPipeline p(chunk);
    }
    SECTION("experimental")
    {
        uint8_t buffer[128];
        MemoryChunk _chunk(buffer, 128);
        BufferProviderPipeline p(_chunk);
        MemoryChunk* chunk = NULLPTR;

        if(p.get_buffer(&chunk))
        {
            sprintf((char*)chunk->data(), "Hello World");

            PipelineMessage::CopiedStatus copied_status;

            MemoryChunk chunk2 = p.advance(100, copied_status);
        }

#ifdef IDEALIZED
        PipelineType p;
        PipelineMessageWrapper<default_boundary_number> msg;

        size_t len = sprintf(msg, "Hello");

        // bump forward and notify interested parties
        msg.advance(len, boundary_number);

        // this should auto advance
        msg << "Hello Again" << endl;

        msg.boundary();

#endif
    }
    SECTION("Pipeline Encoder Adapter")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter p(chunk);
        BasicTestEncoder encoder;
        PipelineEncoderAdapter<BasicTestEncoder> adapter(p, encoder);

        // Not ready for testing just yet
        adapter.process();

        REQUIRE(chunk[0] == 1);
        REQUIRE(chunk[9] == 10);
    }
    SECTION("Buffered Pipeline Encoder Adapter")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter p(chunk);
        BasicTestEncoder encoder;

        BufferedPipelineEncoderAdapter<BasicTestEncoder> adapter(p, encoder);

        // Not ready for testing just yet
        adapter.process();

        REQUIRE(chunk[0] == 1);
        REQUIRE(chunk[9] == 10);
    }
}