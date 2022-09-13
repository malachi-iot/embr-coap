// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>
#include <estd/functional.h>

#include "metadata.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// A connection tracker of sorts.  Does not participate in any scheduling
// DEBT: Tracker not threadsafe, but will need to be for FreeRTOS operation
template <class TTimePoint, class TTransport>
struct Tracker
{
    typedef TTransport transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef TTimePoint time_point;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef Item<endpoint_type, TTimePoint, const_buffer_type> item_base;

    // DEBT: This violates separation of concerns, effectively putting 'Manager' code into
    // 'Tracker'.  Likely we'll want to solve this by passing in a TImpl
    struct item_type : item_base
    {
        typedef item_base base_type;
        typedef Tracker parent_type;

        parent_type* const parent;

        // If ack_received or retransmission counter passes threshold,
        //
        void resend(time_point* p, time_point p2)
        {
            // DEBT: 'delete this' inside here works, but is easy to get wrong.

            if(base_type::ack_received())
            {
                delete this;
                return;
            }

            // DEBT: Still don't like direct transport interaction here (Tracker
            // knowing way too much)
            // DEBT: pbuf's maybe-kinda demand non const
            //transport_type::send(item_base::buffer(), item_base::endpoint());
            transport_type::send(item_base::buffer_, item_base::endpoint());

            // If retransmit counter is within threshold.  See [1] Section 4.2
            if(++base_type::retransmission_counter < COAP_MAX_RETRANSMIT)
            {
                // Reschedule
                *p += base_type::delta();
            }
            else
            {
                // Current architecture we need to remove this from both the vector AND do
                // a delete manually when retransmit counter exceeds threshold
                parent->untrack(this);
                delete this;
            }
        }

        typename estd::internal::thisify_function<void(time_point*, time_point)>::
            template model<item_type, &item_type::resend> m;

        //ESTD_CPP_FORWARDING_CTOR(Item2);

        item_type(parent_type* parent, endpoint_type e, time_point t, const_buffer_type b) :
            parent(parent),
            base_type(e, t, b),
            m(this)
        {

        }
    };

    // DEBT: Replace this with a proper memory pool
    // DEBT: Doing this as item_type* because memory location needs to be fixed
    // for estd::detail::function to find its model
    typedef estd::layer1::vector<item_type*, 10> vector_type;

private:
    vector_type tracked;

public:

    typedef typename vector_type::iterator iterator;
    typedef typename vector_type::const_iterator const_iterator;

    const_iterator end() const { return tracked.end(); }

    bool empty() const { return tracked.empty(); }

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

        auto i = new item_type(this, endpoint, time_sent, buffer);
        tracked.push_back(i);
        return i;
    }

    iterator match(const endpoint_type& endpoint, uint16_t mid)
    {
        auto it = estd::find_if(tracked.begin(), tracked.end(), [&](const item_type* v)
        {
            //return endpoint == v.endpoint() && mid == v.mid();
            return endpoint == v->endpoint() && mid == v->mid();
        });
        return it;
    }

    void untrack(iterator it)
    {
        tracked.erase(it);

        // NOTE: Cannot delete this here because scheduler still wants to pick up model contained
        // in 'it'
        //delete it.lock();
    }

    void untrack(const item_type* item)
    {
        // DEBT: Add https://en.cppreference.com/w/cpp/algorithm/remove and consider using
        // that here
        auto it = match(item->endpoint(), item->mid());

        if(it == tracked.end())
            return;

        untrack(it);
    }
};


}}}}