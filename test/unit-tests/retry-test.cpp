#include <catch.hpp>

#include <exp/retry.h>

#include "test-data.h"
#include "test-uri-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;

// using "native" literals makes compilers mad sometimes.
// Using explicit seconds, milliseconds, etc
//using namespace estd::chrono_literals;

struct SyntheticTransport
{
    typedef unsigned endpoint_type;
    typedef estd::span<uint8_t> buffer_type;

    // EXPERIMENTAL
    // Needed because span itself isn't const, but stuff it points to is.  So
    // regular add/remove cv doesn't quite apply
    typedef estd::span<const uint8_t> const_buffer_type;
};

struct SyntheticClock
{
    typedef estd::chrono::time_point<SyntheticClock, estd::chrono::milliseconds> time_point;
};

TEST_CASE("retry tests", "[retry]")
{
    SECTION("experimental v4 retry")
    {
        // DEBT: Doesn't play nice because std::chrono::time_point doesn't like converting
        // to estd::chrono::time_point (or is it vice versa?)
        //typedef typename estd::chrono::steady_clock::time_point time_point;

        typedef estd::chrono::time_point<SyntheticClock, estd::chrono::milliseconds> time_point;
        typedef embr::internal::layer1::Scheduler<8, embr::internal::scheduler::impl::Function<time_point> > scheduler_type;
        typedef estd::span<const uint8_t> buffer_type;
        time_point zero_time;

        scheduler_type scheduler;

        buffer_type b(buffer_simplest_request);

        SECTION("factory")
        {
            typedef DecoderFactory<buffer_type> factory;

            SECTION("core")
            {
                auto d = factory::create(b);

                auto r = d.process_iterate_streambuf();
                r = d.process_iterate_streambuf();

                REQUIRE(d.state() == Decoder::HeaderDone);

                Header h = d.header_decoder();

                REQUIRE(h.message_id() == 0x123);
            }
            SECTION("helper")
            {
                Header h = get_header(b);

                REQUIRE(h.message_id() == 0x123);
            }
        }
        SECTION("tracker")
        {
            typedef retry::Tracker<unsigned, time_point, buffer_type> tracker_type;

            tracker_type tracker;

            SECTION("core")
            {
                auto i = tracker.track(0, zero_time, b);

                auto match = tracker.match(0, 0x123);

                REQUIRE(match != tracker.tracked.end());

                match = tracker.match(0, 0x1234);

                REQUIRE(match == tracker.tracked.end());

                tracker.untrack(i);
            }
        }
        SECTION("manager")
        {
            retry::Manager<SyntheticClock, SyntheticTransport> manager;

            SECTION("scheduled")
            {
                manager.send(0, zero_time, b, scheduler);

                time_point t(estd::chrono::seconds(5));

                scheduler.process(t);
            }
        }
    }
}