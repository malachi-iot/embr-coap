#include <catch.hpp>

#include <exp/retry.h>

#include "test-data.h"
#include "test-uri-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;

// using "native" literals makes compilers mad sometimes.
// Using explicit seconds, milliseconds, etc
//using namespace estd::chrono_literals;

// NOTE: Remember, embr's Transport concept wants send with signature
// 'buffer', 'endpoint' - lwip style.  In retrospect I would have preferred
// 'endpoint', 'buffer'
struct SyntheticTransport
{
    typedef unsigned endpoint_type;
    typedef estd::span<uint8_t> buffer_type;

    // EXPERIMENTAL
    // Needed because span itself isn't const, but stuff it points to is.  So
    // regular add/remove cv doesn't quite apply
    typedef estd::span<const uint8_t> const_buffer_type;

    struct Item
    {
        endpoint_type e;
        const_buffer_type b;
    };

    static Item last_sent;
    static int counter;

    static void send(const_buffer_type b, endpoint_type e)
    {
        ++counter;
        last_sent = Item{e, b};
    }
};

// DEBT: Don't think I like a static here, even in a synthetic sense
SyntheticTransport::Item SyntheticTransport::last_sent;
int SyntheticTransport::counter = 0;

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
            typedef retry::Tracker<time_point, SyntheticTransport> tracker_type;

            tracker_type tracker;

            SECTION("core")
            {
                auto i = tracker.track(0, zero_time, b);

                auto match = tracker.match(0, 0x123);

                REQUIRE(match != tracker.end());

                match = tracker.match(0, 0x1234);

                REQUIRE(match == tracker.end());

                tracker.untrack(i);
            }
        }
        SECTION("manager")
        {
            retry::Manager<SyntheticClock, SyntheticTransport> manager;
            const time_point t2(estd::chrono::seconds(2));
            const time_point t3(estd::chrono::seconds(3));
            const time_point t5(estd::chrono::seconds(5));

            SECTION("ACK received immediately")
            {
                SyntheticTransport::counter = 0;

                manager.send(1, zero_time, b, scheduler);

                bool result = manager.on_received(1, buffer_ack);

                REQUIRE(result);

                scheduler.process(t5);

                // NOTE: For vector flavor this is true right after on_received.
                // For upcoming linked list version, we'll need to process first
                REQUIRE(manager.tracker.tracked.empty());

                REQUIRE(SyntheticTransport::last_sent.e == 1);
                REQUIRE(SyntheticTransport::counter == 1);
            }
            SECTION("other")
            {
                SyntheticTransport::counter = 0;

                manager.send(1, zero_time, b, scheduler);

                scheduler.process(t2);

                REQUIRE(SyntheticTransport::counter == 1);

                scheduler.process(t5);

                // At this point 'process' has sent a 2nd packet
                REQUIRE(SyntheticTransport::counter == 2);
            }
        }
    }
}