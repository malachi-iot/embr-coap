#include <catch.hpp>

#include <exp/retry.h>

#include "test-data.h"
#include "test-uri-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;

TEST_CASE("retry tests", "[retry]")
{
    SECTION("experimental v4 retry")
    {
        typedef estd::span<const uint8_t> buffer_type;

        SECTION("factory")
        {
            typedef DecoderFactory<buffer_type> factory;

            buffer_type b(buffer_simplest_request);

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
            typedef retry::Tracker<unsigned, unsigned, buffer_type> tracker_type;

            tracker_type tracker;

            buffer_type b(buffer_simplest_request);

            auto i = tracker.track(0, 0, b);

            auto match = tracker.match(0, 0x123);

            REQUIRE(match != tracker.tracked.end());

            match = tracker.match(0, 0x1234);

            REQUIRE(match == tracker.tracked.end());

            tracker.untrack(i);
        }
    }
}