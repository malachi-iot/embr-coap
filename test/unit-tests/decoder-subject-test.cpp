#include <catch.hpp>
#include "coap/decoder-subject.hpp"
#include "test-data.h"

using namespace moducom::coap;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
static IncomingContext test_ctx;
static DecoderSubjectBase<experimental::ContextDispatcherHandler> test(test_ctx);
// ---

// FIX: putting this above causes compilation issues, clean that up
#include "test-observer.h"

using namespace moducom::pipeline;

TEST_CASE("CoAP decoder subject tests", "[coap-decoder-subject]")
{
    SECTION("16 bit delta: ContextDispatcherHandler")
    {
        MemoryChunk::readonly_t chunk(buffer_16bit_delta);
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
    SECTION("16 bit detla: Buffer16BitDeltaObserver")
    {
        IncomingContext test_ctx2;
        DecoderSubjectBase<Buffer16BitDeltaObserver> test2(test_ctx2);
        // FIX: on_payload not evaluating for these tests, not surprising
        // since subject-decoder is revamping how payload processing works
        MemoryChunk::readonly_t chunk(buffer_16bit_delta);
        test2.dispatch(chunk);
    }
    SECTION("16 bit detla: Buffer16BitDeltaObserver&")
    {
        Buffer16BitDeltaObserver observer;
        DecoderSubjectBase<IMessageObserver&> test2(observer);
        MemoryChunk::readonly_t chunk(buffer_16bit_delta);
        test2.dispatch(chunk);
    }
    SECTION("payload only")
    {
        MemoryChunk::readonly_t chunk(buffer_payload_only);

        // TODO: Make an observer to evaluate that payload is appearing as expected
        test.reset();
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
    SECTION("'plausible' coap buffer")
    {
        MemoryChunk::readonly_t chunk(buffer_plausible);

        test.reset();
        test.dispatch(chunk);
    }
}
