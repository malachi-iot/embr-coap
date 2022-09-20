#pragma once

#include <estd/chrono.h>

#include <embr/platform/lwip/transport.h>
#include <embr/scheduler.h>

#include <exp/retry.h>  // for get_header, manager


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

