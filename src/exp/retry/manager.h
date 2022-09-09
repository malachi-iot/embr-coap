// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>

#include <embr/scheduler.h>

#include "tracker.h"

namespace embr { namespace coap { namespace experimental { namespace retry {



// DEBT: TTransport = TTransportDescriptor
template <class TClock, class TTransport>
struct Manager
{
    typedef TTransport transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef TClock clock_type;
    typedef typename clock_type::time_point time_point;

    typedef Tracker<endpoint_type, time_point, const_buffer_type> tracker_type;
    typedef typename tracker_type::item_type item_type;

    typedef embr::internal::scheduler::impl::Function<time_point> scheduler_impl;


    tracker_type tracker;

    // NOT USED YET
    template <class TContainer, class TSubject>
    const item_type* send(const endpoint_type& endpoint, time_point time_sent, const_buffer_type buffer,
        embr::internal::Scheduler<TContainer, scheduler_impl, TSubject>& scheduler)
    {
        const item_type* i = tracker.track(endpoint, time_sent, buffer);

        //time_point now = clock_type::now();   // TODO
        time_point due = time_sent + i->delta();

        // 9 billion errors, none of them clear
        //estd::detail::function<void(time_point*, time_point)> f(&i->m);

        // NOTE: Can't use thisafy and friends because 'this' pointer is getting moved around
        // So in the short term we need a true dynamic allocation
        //scheduler.schedule(due, f);

        return i;
    }
};

}}}}