/**
 * @file
 * @author  Malachi Burke
 */
#pragma once

#include <catch.hpp>

#include <obsolete/coap-dispatcher.h>
#include <obsolete/coap/decoder/subject.h> // for event definitions
#include <obsolete/coap/experimental-observer.h>
#include <coap/decoder/netbuf.h>
#include <exp/netbuf.h>
#include "test-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;
using namespace moducom::pipeline;

/// @brief designed specifically to test against "buffer_16bit_delta" buffer
/// @remarks the whole DecoderObserverBase chain is deprecated
/// @deprecated
/// \tparam TRequestContext
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


template <class TIncomingContext>
class EmptyObserver : public MessageObserverBase <TIncomingContext>
{
public:
};


// simplistic memorychunk-mapped NetBuf.  Eventually put this into mcmem itself
// FIX: this one is hardwired to read operations - not a crime, but needs more
// architectual planning to actually be used elsewhere
class NetBufReadOnlyMemory :
        public moducom::io::experimental::NetBufMemoryTemplate<moducom::pipeline::MemoryChunk::readonly_t >
{
    typedef moducom::pipeline::MemoryChunk::readonly_t chunk_t;
    typedef moducom::io::experimental::NetBufMemoryTemplate<chunk_t> base_t;

public:
    template <size_t N>
    NetBufReadOnlyMemory(const uint8_t (&a) [N]) :
        base_t(chunk_t(a, N))
    {

    }

    NetBufReadOnlyMemory(const NetBufReadOnlyMemory& copy_from) :
        base_t(copy_from)
    {

    }

#ifdef FEATURE_CPP_MOVESEMANTIC
#endif

    // FIX: hard wiring this to a read-oriented NetBuf
    // there's some mild indication this isn't ideal since conjunctive decoder
    // still needs to maintain a pos to read through this
    size_t length_processed() const
    {
        return base_t::_chunk.length();
    }
};


struct test_observer
{
    int counter;

    test_observer() : counter(0) {}

    void on_notify(event::completed)
    {
        REQUIRE(counter >= 2);
        counter++;
    }

    void on_notify(const event::option& e)
    {
        REQUIRE(e.option_number >= 270);
        counter++;
    }
};


