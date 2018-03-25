#include <catch.hpp>
#include "../coap/decoder-subject.hpp"
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
//static DecoderSubjectBase<Buffer16BitDeltaObserver> test2(test_ctx);

TEST_CASE("CoAP decoder subject tests", "[coap-decoder-subject]")
{
    SECTION("16 bit delta")
    {
        MemoryChunk::readonly_t chunk(buffer_16bit_delta);
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
    SECTION("payload only")
    {
        MemoryChunk::readonly_t chunk(buffer_payload_only);
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
}
