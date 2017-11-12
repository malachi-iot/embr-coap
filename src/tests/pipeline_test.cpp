//
// Created by Malachi Burke on 11/12/17.
//

#include <catch.hpp>

#include "../mc/pipeline.h"

using namespace moducom::pipeline;
using namespace moducom::pipeline::experimental;
using namespace moducom::pipeline::layer3;
using namespace moducom::pipeline::layer3::experimental;

TEST_CASE("Pipeline basic tests", "[pipeline]")
{
    SECTION("1")
    {
        MemoryChunk chunk;
        SimpleBufferedPipeline p(chunk);
    }
    SECTION("experimental")
    {
        BufferProviderPipeline p;
        MemoryChunk* chunk = NULLPTR;

        if(p.get_buffer(&chunk))
        {
            PipelineMessage::CopiedStatus copied_status;

            MemoryChunk chunk2 = p.advance(100, copied_status);
        }
    }
}