#pragma once

#include <vector>

#include <estd/chrono.h>

#include <embr/platform/lwip/transport.h>
#include <embr/scheduler.h>

#include <exp/retry.h>  // for get_header, manager

#define FEATURE_RETRY_MANAGER       0
#define FEATURE_RETRY_TRACKER_V2    1

// DEBT: Eventually we'll support both at once, but for now that's undefined behavior
#define CONFIG_UDP      0
#define CONFIG_ESP_NOW  1


// DEBT: const_buffer_type still in flux.  Once that settles down,
// push this up into TransportUdp itself
// DEBT: TransportUdp still kind of incomplete feeling, but it's proven
// itself enough to upgrade from experimental to internal
struct transport_type : embr::lwip::experimental::TransportUdp<>
{
    typedef embr::lwip::experimental::TransportUdp<> base_type;
    typedef typename base_type::buffer_type const_buffer_type;

    ESTD_CPP_FORWARDING_CTOR(transport_type);
};

typedef estd::chrono::freertos_clock clock_type;
typedef typename clock_type::time_point time_point;
typedef embr::internal::layer1::Scheduler<8, embr::internal::scheduler::impl::Function<time_point> > scheduler_type;
typedef embr::coap::experimental::retry::Manager<clock_type, transport_type> manager_type;

#if FEATURE_RETRY_TRACKER_V2
namespace app {
using duration = estd::chrono::freertos_clock::duration;
void loop();
}
namespace app::lwip {
using endpoint_type = embr::lwip::internal::Endpoint<false>;
using buffer_type = embr::lwip::v1::Pbuf;

using tracker_type = embr::coap::experimental::retry::Tracker2<endpoint_type, buffer_type>;
extern tracker_type tracker;
}

namespace app::esp_now {
using endpoint_type = unsigned;
using buffer_type = std::vector<uint8_t>;   // DEBT: Prefer from a pool, or, at a minimum, using psram_allocator

using tracker_type = embr::coap::experimental::retry::Tracker2<endpoint_type, buffer_type>;
extern tracker_type tracker;
void init();
void loop(duration);
}
#endif
