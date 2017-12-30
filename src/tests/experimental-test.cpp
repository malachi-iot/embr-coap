#include <catch.hpp>

#include "../coap-decoder.h"
#include "test-data.h"
#include "../mc/experimental.h"
//#include "../mc/pipeline.h"

using namespace moducom::coap;
//using namespace moducom::pipeline;

TEST_CASE("experimental tests", "[experimental]")
{
    SECTION("1")
    {
        experimental::layer1::ProcessedMemoryChunk<128> chunk;

        chunk[0] = 1;
        chunk.processed(1);
    }
}