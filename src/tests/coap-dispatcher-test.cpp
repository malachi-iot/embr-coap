//
// Created by malachi on 12/27/17.
//

#include <catch.hpp>

#include "../coap-dispatcher.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;

TEST_CASE("CoAP dispatcher tests", "[coap-dispatcher]")
{
    SECTION("Test 1")
    {
        Dispatcher dispatcher;
        //layer3::MemoryChunk<128> chunk;
        MemoryChunk chunk(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        dispatcher.dispatch(chunk);


    }
}