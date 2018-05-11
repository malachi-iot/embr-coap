#pragma once

#include <catch.hpp>

#include "coap-dispatcher.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;

// designed specifically to test against "buffer_16bit_delta" buffer
template <class TRequestContext>
class Buffer16BitDeltaObserver : public DecoderObserverBase<TRequestContext>
{
    int option_test_number;

    typedef DecoderObserverBase<TRequestContext> base_t;

public:
    typedef TRequestContext request_context_t;
    typedef typename base_t::number_t number_t;
    typedef typename base_t::InterestedEnum interested_t;

    Buffer16BitDeltaObserver(request_context_t& dummy) :
        base_t(base_t::Always),
        option_test_number(0) {}

    Buffer16BitDeltaObserver(interested_t i = base_t::Always) :
        base_t(i),
        option_test_number(0) {}

    virtual void on_header(Header header) OVERRIDE
    {
        REQUIRE(header.is_request());
    }

    virtual void on_token(const MemoryChunk::readonly_t& token_part, bool last_chunk) OVERRIDE
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

    virtual void on_option(number_t number, const MemoryChunk::readonly_t& option_value_part, bool last_chunk) OVERRIDE
    {
        switch(option_test_number++)
        {
            case 0:
                REQUIRE(option_value_part[0] == 3);
                REQUIRE(option_value_part.length() == 1);
                break;

            case 1:
                REQUIRE(option_value_part[0] == 4);
                REQUIRE(option_value_part[1] == 5);
                REQUIRE(option_value_part.length() == 2);
                break;
        }
    }

    virtual void on_payload(const MemoryChunk::readonly_t& payload_part, bool last_chunk) OVERRIDE
    {
        REQUIRE(payload_part[0] == buffer_16bit_delta[12]);
        REQUIRE(payload_part[1] == buffer_16bit_delta[13]);
        REQUIRE(payload_part[payload_part.length()] == buffer_16bit_delta[12 + payload_part.length()]);
    }
};
