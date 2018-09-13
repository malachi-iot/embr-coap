#include <catch.hpp>
#include "coap/decoder/subject.hpp"
#include "coap/decoder/observer-aggregate.hpp"
#include <exp/diagnostic-decoder-observer.h>
#include "test-data.h"
#include "test-observer.h"

#include <estd/vector.h>

#include <embr/observer.h>

using namespace moducom::coap;

typedef TokenAndHeaderContext<true, false> request_context_t;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
static request_context_t test_ctx;
static DecoderSubjectBase<moducom::coap::experimental::ContextDispatcherHandler<request_context_t> > test(test_ctx);
// ---

// FIX: putting this above causes compilation issues, clean that up
#include "test-observer.h"

// NOTE: Next generation subject/observe code, will replace the above EmptyObserver and friends
struct test_static_observer
{
    static int counter;

    static void on_notify(const header_event&) {}
    static void on_notify(const token_event&) {}
    static void on_notify(const payload_event&) {}

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
        // works well but outmoded by 'notify_from_decoder'
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
            embr::void_subject s;

            decoder.process_experimental(s, context);

            /*
            do
            {
                decoder.process_iterate_experimental(s, context);

                //notify_from_decoder(s, decoder, context);
            } while (decoder.state() != Decoder::Done); */
        }
#ifdef FEATURE_CPP_VARIADIC
        SECTION("stateless subject")
        {
            embr::layer0::subject<test_static_observer> s;

            // FIX: this does not get counter to 3, because
            // on_completed event is never fired (process
            // iterates until buffer end, not until state machine yields Decoder::Done)
            //decoder.process_experimental(s, context);

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

            embr::layer0::subject<test_static_observer> s;
            typedef NetBufReadOnlyMemory netbuf_t;
            typedef NetBufDecoder<netbuf_t&> decoder_type;

            netbuf_t nb(buffer_16bit_delta);

            moducom::coap::DecoderContext<decoder_type> dc(nb);

            decoder_type decoder = dc.decoder();

            do
            {
                decode_and_notify(s, decoder, decoder.context);

            } while (decoder.state() != Decoder::Done);

            REQUIRE(decoder_type::has_end_method<netbuf_t>::value);
            REQUIRE(!decoder_type::has_data_method<netbuf_t>::value);
        }
    }
}
