#include <catch.hpp>
#include "coap/decoder/subject.hpp"
#include "coap/decoder/observer-aggregate.hpp"
#include <exp/diagnostic-decoder-observer.h>
#include "test-data.h"
#include "test-observer.h"

#include <estd/vector.h>
#include <estd/exp/observer.h>

using namespace moducom::coap;

typedef TokenAndHeaderContext<true, false> request_context_t;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
static request_context_t test_ctx;
static DecoderSubjectBase<moducom::coap::experimental::ContextDispatcherHandler<request_context_t> > test(test_ctx);
// ---

// FIX: putting this above causes compilation issues, clean that up
#include "test-observer.h"

// FIX: Nevermind, because inheritence ends up hiding overloaded names,
// these dummy base classes won't help
struct dummy_observer
{
    void on_notify(const header_event& e) {}
    void on_notify(const token_event& e) {}
    void on_notify(const option_event& e) {}
    void on_notify(const payload_event& e) {}
    void on_notify(const completed_event& e) {}
};


struct dummy_static_observer
{
    static void on_notify(const header_event& e) {}
    static void on_notify(const token_event& e) {}
    static void on_notify(const option_event& e) {}
    static void on_notify(const payload_event& e) {}
    static void on_notify(const completed_event& e) {}
};


// NOTE: Next generation subject/observe code, will replace the above EmptyObserver and friends
struct test_static_observer
{
    static int counter;

    static void on_notify(const header_event& e) {}
    static void on_notify(const token_event& e) {}
    static void on_notify(const payload_event& e) {}

    static void on_notify(const completed_event&)
    {
        REQUIRE(counter == 2);
        counter++;
    }

    static void on_notify(const option_event& e)
    {
        REQUIRE(e.option_number >= 270);
        counter++;
    }
};


int test_static_observer::counter = 0;

using namespace moducom::pipeline;

TEST_CASE("CoAP decoder subject tests", "[coap-decoder-subject]")
{
    typedef estd::const_buffer ro_chunk_t;

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
    SECTION("new estd::experimental::subject flavor of things")
    {
        ro_chunk_t chunk(buffer_16bit_delta);
        Decoder decoder;
        Decoder::Context context(chunk, true);

        SECTION("void subject")
        {
            estd::experimental::void_subject s;
            do
            {
                notify_from_decoder(s, decoder, context);
            } while (decoder.state() != Decoder::Done);
        }
#ifdef FEATURE_CPP_VARIADIC
        SECTION("stateless subject")
        {
            estd::experimental::internal::stateless_subject<test_static_observer> s;
            do
            {
                notify_from_decoder(s, decoder, context);
            } while (decoder.state() != Decoder::Done);
            REQUIRE(test_static_observer::counter == 3);
        }
#endif
        SECTION("DecoderContext / subject observe test")
        {
            test_static_observer::counter = 0;

            estd::experimental::internal::stateless_subject<test_static_observer> s;
            typedef NetBufReadOnlyMemory netbuf_t;

            netbuf_t nb(buffer_16bit_delta);

            moducom::coap::DecoderContext<NetBufDecoder<netbuf_t&>> dc(nb);

            NetBufDecoder<netbuf_t&> decoder = dc.decoder();

            do
            {
                notify_from_decoder(s, decoder, decoder.context);
            } while (decoder.state() != Decoder::Done);
        }
    }
}
