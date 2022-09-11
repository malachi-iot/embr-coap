// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>
#include <estd/functional.h>

#include "metadata.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// A connection tracker of sorts.  Does not participate in any scheduling
template <class TTimePoint, class TTransport>
struct Tracker
{
    typedef TTransport transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef TTimePoint time_point;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef Item<endpoint_type, TTimePoint, const_buffer_type> item_base;

    struct item_type : item_base
    {
        typedef item_base base_type;

        void resend(time_point* p, time_point p2)
        {
            // DEBT: Still don't like direct transport interaction here
            transport_type::send(item_base::endpoint(), item_base::buffer());
        }

        // DEBT: This violates separation of concerns, effectively putting 'Manager' code into
        // 'Tracker'
        typename estd::internal::thisify_function<void(time_point*, time_point)>::
            template model<item_type, &item_type::resend> m;

        //ESTD_CPP_FORWARDING_CTOR(Item2);

        item_type(endpoint_type e, time_point t, const_buffer_type b) :
            base_type(e, t, b),
            m(this)
        {

        }
    };

    // DEBT: Replace this with a proper memory pool
    // DEBT: Doing this as item_type* because memory location needs to be fixed
    // for estd::detail::function to find its model
    estd::layer1::vector<item_type*, 10> tracked;

    ~Tracker()
    {
        for(item_type* i : tracked)
            delete i;
    }

    const item_type* track(const endpoint_type& endpoint, time_point time_sent, const_buffer_type buffer)
    {
        /*
        item_type _i{endpoint};
        tracked.push_back(_i);
        const item_type& i = tracked.back();
        return &i; */

        //const item_type& i = tracked.emplace_back(endpoint, time_sent, buffer);
        //return &i;

        auto i = new item_type(endpoint, time_sent, buffer);
        tracked.push_back(i);
        return i;
    }

    decltype(tracked.begin()) match(const endpoint_type& endpoint, uint16_t mid)
    {
        auto it = estd::find_if(tracked.begin(), tracked.end(), [&](const item_type* v)
        {
            //return endpoint == v.endpoint() && mid == v.mid();
            return endpoint == v->endpoint() && mid == v->mid();
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
        delete item;
    }
};


}}}}