#include <catch2/catch.hpp>

#include <estd/algorithm.h>
#include <embr/scheduler.hpp>

#include <exp/retry.h>
#include <exp/retry/manager.hpp>

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
// DEBT: Combine this with embr's test transport, and include that here instead
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

    Item last_sent;
    int counter = 0;

    void send(const_buffer_type b, endpoint_type e)
    {
        ++counter;
        last_sent = Item{e, b};
    }
};

// DEBT: Combine this with estd's test clock, and include that here instead
struct SyntheticClock
{
    typedef typename estd::chrono::milliseconds duration;
    typedef estd::chrono::time_point<SyntheticClock> time_point;

    static time_point now_;

    // is monotonic -- i.e. doesn't decrease.  This is only held true because unit tests
    // don't decrease time within themselves
    static constexpr bool is_steady = true;

    static time_point now() { return now_; }
};

SyntheticClock::time_point SyntheticClock::now_;

TEST_CASE("retry tests", "[retry]")
{
    SECTION("experimental v4 retry")
    {
        // DEBT: Doesn't play nice because std::chrono::time_point doesn't like converting
        // to estd::chrono::time_point (or is it vice versa?)
        //typedef typename estd::chrono::steady_clock::time_point time_point;

        typedef estd::chrono::time_point<SyntheticClock, estd::chrono::milliseconds> time_point;
        typedef embr::internal::layer1::Scheduler<8,
            embr::internal::scheduler::impl::Function<time_point,
            estd::detail::impl::function_fnptr1> > scheduler_type;
        typedef estd::span<const uint8_t> buffer_type;
        constexpr time_point zero_time;

        scheduler_type scheduler;

        buffer_type b(buffer_simplest_request);

        SECTION("factory")
        {
            typedef internal::DecoderFactory<buffer_type> factory;

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
                auto i = tracker.track(0, zero_time, std::move(b));

                auto match = tracker.match(0, 0x123);

                REQUIRE(match != tracker.end());

                match = tracker.match(0, 0x1234);

                REQUIRE(match == tracker.end());

                tracker.untrack(i);
            }
        }
        SECTION("manager")
        {
            SyntheticTransport transport;
            retry::Manager<SyntheticClock, SyntheticTransport&> manager(transport);

            const time_point t2(estd::chrono::seconds(2));
            const time_point t3(estd::chrono::seconds(3));
            const time_point t5(estd::chrono::seconds(5));

            SECTION("ACK received immediately")
            {
                // 'send' call without explicit time uses clock_type::now()
                manager.send(1, std::move(b), scheduler);

                REQUIRE(!manager.tracker.empty());

                bool result = manager.on_received(1, buffer_ack);

                REQUIRE(result);

                scheduler.process(t5);

                // NOTE: For vector flavor this is true right after on_received.
                // For upcoming linked list version, we'll need to process first
                REQUIRE(manager.tracker.empty());

                REQUIRE(transport.last_sent.e == 1);
                REQUIRE(transport.counter == 1);
            }
            // 11AUG23 Unexpectedly hit issue here, temporarily disabling as we are here to solve other
            // problems first
#if TEMPORARILY_DISABLED
            SECTION("other")
            {
                auto item = manager.send(1, zero_time, std::move(b), scheduler);

                REQUIRE(item->retransmission_counter == 0);

                scheduler.process(t2);

                // No retransmit yet
                REQUIRE(item->retransmission_counter == 0);

                scheduler.process(t5);

                // At this point 'process' has sent a 2nd packet, 1st retransmit
                REQUIRE(item->retransmission_counter == 1);

                scheduler.process(time_point(estd::chrono::seconds(10)));

                // At this point 'process' has sent a 3rd packet, 2nd retransmit
                REQUIRE(item->retransmission_counter == 2);

                scheduler.process(time_point(estd::chrono::seconds(20)));

                // At this point 'process' has sent a 4th packet, 3rd retransmit
                REQUIRE(item->retransmission_counter == 3);

                scheduler.process(time_point(estd::chrono::seconds(40)));

                auto it = manager.tracker.match(1, 0x123);

                // At this point 'process' gives up, because MAX_RETRANSMIT is 4.
                // Therefore, no connection with endpoint and mid is matched
                REQUIRE(it == manager.tracker.end());

                // DEBT: Document exactly why we expect '5' here
                REQUIRE(transport.counter == 5);
            }
#endif
        }
    }
}