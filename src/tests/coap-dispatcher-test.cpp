//
// Created by malachi on 12/27/17.
//

#include <catch.hpp>

#include "../coap-dispatcher.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;

class TestDispatcherHandler : public IDispatcherHandler
{
public:
    virtual void on_header(CoAP::Header header) OVERRIDE
    {

    }

    virtual void on_token(const MemoryChunk& token_part, bool last_chunk) OVERRIDE
    {

    }

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {

    }

    virtual void on_option(const MemoryChunk& option_value_part, bool last_chunk) OVERRIDE
    {

    }

    virtual void on_payload(const MemoryChunk& payload_part, bool last_chunk) OVERRIDE
    {

    }

    virtual InterestedEnum interested() OVERRIDE
    {
        return Always;
    }

public:
};

TEST_CASE("CoAP dispatcher tests", "[coap-dispatcher]")
{
    SECTION("Test 1")
    {
        TestDispatcherHandler dispatcherHandler;
        Dispatcher dispatcher;
        //layer3::MemoryChunk<128> chunk;
        MemoryChunk chunk(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        dispatcher.head(&dispatcherHandler);
        dispatcher.dispatch(chunk);


    }
}