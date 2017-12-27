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
    int option_test_number;

public:
    TestDispatcherHandler() : option_test_number(0) {}

    virtual void on_header(CoAP::Header header) OVERRIDE
    {
        REQUIRE(header.is_request());
    }

    virtual void on_token(const MemoryChunk& token_part, bool last_chunk) OVERRIDE
    {
        FAIL("Should not reach here");
    }

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {
        switch(option_test_number)
        {
            case 0:
                REQUIRE(number == 270);
                REQUIRE(length == 1);
                break;

            case 1:
                REQUIRE(number == 271);
                REQUIRE(length == 2);
                break;
        }
    }

    virtual void on_option(const MemoryChunk& option_value_part, bool last_chunk) OVERRIDE
    {
        switch(option_test_number++)
        {
            case 0:
                REQUIRE(option_value_part[0] == 3);
                REQUIRE(option_value_part.length == 1);
                break;

            case 1:
                REQUIRE(option_value_part[0] == 4);
                REQUIRE(option_value_part[1] == 5);
                REQUIRE(option_value_part.length == 2);
                break;
        }
    }

    virtual void on_payload(const MemoryChunk& payload_part, bool last_chunk) OVERRIDE
    {
        REQUIRE(payload_part[0] == buffer_16bit_delta[12]);
        REQUIRE(payload_part[payload_part.length] == buffer_16bit_delta[12 + payload_part.length]);
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