#include <catch.hpp>
#include "coap/decoder/subject.hpp"
#include "coap/decoder/observer-aggregate.hpp"
#include "test-data.h"

#include <estd/vector.h>

using namespace moducom::coap;

typedef TokenAndHeaderContext<true> request_context_t;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
static request_context_t test_ctx;
static DecoderSubjectBase<experimental::ContextDispatcherHandler<request_context_t> > test(test_ctx);
// ---

// FIX: putting this above causes compilation issues, clean that up
#include "test-observer.h"

using namespace moducom::pipeline;

TEST_CASE("CoAP decoder subject tests", "[coap-decoder-subject]")
{
    typedef estd::experimental::const_buffer ro_chunk_t;

    SECTION("16 bit delta: ContextDispatcherHandler")
    {
        ro_chunk_t chunk(buffer_16bit_delta);
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
    SECTION("16 bit detla: Buffer16BitDeltaObserver")
    {
        request_context_t test_ctx2;
        DecoderSubjectBase<Buffer16BitDeltaObserver<request_context_t> > test2(test_ctx2);
        // FIX: on_payload not evaluating for these tests, not surprising
        // since subject-decoder is revamping how payload processing works
        ro_chunk_t chunk(buffer_16bit_delta);
        test2.dispatch(chunk);
    }
    SECTION("16 bit detla: Buffer16BitDeltaObserver&")
    {
        Buffer16BitDeltaObserver<ObserverContext> observer;
        DecoderSubjectBase<IMessageObserver&> test2(observer);
        ro_chunk_t chunk(buffer_16bit_delta);
        test2.dispatch(chunk);
    }
    SECTION("payload only")
    {
        ro_chunk_t chunk(buffer_payload_only);

        // TODO: Make an observer to evaluate that payload is appearing as expected
        test.reset();
        test.dispatch(chunk);

        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().is_request());
    }
    SECTION("'plausible' coap buffer")
    {
        ro_chunk_t chunk(buffer_plausible);

        test.reset();
        test.dispatch(chunk);
    }
    SECTION("Aggregate observer (runtime-ish version)")
    {
        IDecoderObserver<ObserverContext>* preset[10] = { NULLPTR };
        //IDecoderObserver<ObserverContext>** helper = preset;
        //AggregateDecoderObserver<ObserverContext, estd::layer2::vector<IDecoderObserver<ObserverContext>*, 10> >
          //      ado = preset;
    }
}
