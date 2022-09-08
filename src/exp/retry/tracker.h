// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>

#include <embr/scheduler.h>

#include "metadata.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// A connection tracker of sorts.  Does not participate in any scheduling
template <class TEndpoint, class TTimePoint, class TBuffer>
struct Tracker
{
    typedef TEndpoint endpoint_type;
    typedef TTimePoint time_point;
    typedef TBuffer buffer_type;

    typedef embr::internal::scheduler::impl::Function<time_point> scheduler_impl;

    typedef Item<TEndpoint, TTimePoint, TBuffer> item_type;

    // DEBT: Replace this with a proper memory pool
    estd::layer1::vector<item_type, 10> tracked;

    const item_type* track(const endpoint_type& endpoint, time_point time_sent, buffer_type buffer)
    {
        /*
        item_type _i{endpoint};
        tracked.push_back(_i);
        const item_type& i = tracked.back();
        return &i; */

        const item_type& i = tracked.emplace_back(endpoint, buffer);

        return &i;
    }

    // NOT USED YET
    template <class TContainer, class TSubject>
    const item_type* track(const endpoint_type& endpoint, time_point time_sent, buffer_type buffer,
        embr::internal::Scheduler<TContainer, scheduler_impl, TSubject>& scheduler)
    {
        const item_type* i = track(endpoint, time_sent, buffer);

        time_point due = time_sent + i->delta();

        // NOTE: Can't use thisafy and friends because 'this' pointer is getting moved around
        //scheduler.schedule()
    }

    decltype(tracked.begin()) match(const endpoint_type& endpoint, uint16_t mid)
    {
        auto it = estd::find_if(tracked.begin(), tracked.end(), [&](const item_type& v)
        {
            return endpoint == v.endpoint() && mid == v.mid();
        });
        return it;
    }

    void untrack(const item_type* item)
    {
        // DEBT: Add https://en.cppreference.com/w/cpp/algorithm/remove and consider using
        // that here
        auto it = match(item->endpoint(), item->mid());

        if(it == tracked.end())
            return;

        tracked.erase(it);
    }
};

}}}}