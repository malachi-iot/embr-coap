// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>
#include <estd/functional.h>

#include "metadata.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// A connection tracker of sorts.  Does not participate in any scheduling
// TItem is more or less an impl pattern to help avoid virtual functions
// DEBT: Tracker not threadsafe, but will need to be for FreeRTOS operation
// DEBT: Can probably unwind TTransport and pass endpoint/buffer in directly again
template <class TTimePoint, class TTransport,
    // DEBT: Might be able to do an Item with TTransport specialization.
    // Remember also this default class is for reference
    class TItem = Item<
        typename TTransport::endpoint_type,
        TTimePoint,
        typename TTransport::const_buffer_type> >
struct Tracker
{
    typedef TTransport transport_type;
    typedef typename transport_type::endpoint_type endpoint_type;
    typedef TTimePoint time_point;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef TItem item_type;

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

    // DEBT: Would prefer TArgs prepend, but that breaks typical C++ paradigm
    // for variadic on a method
    template <class ...TArgs>
    const item_type* track(
        const endpoint_type& endpoint,
        time_point time_sent,
        const_buffer_type buffer,
        TArgs...args)
    {
        /*
        item_type _i{endpoint};
        tracked.push_back(_i);
        const item_type& i = tracked.back();
        return &i; */

        //const item_type& i = tracked.emplace_back(endpoint, time_sent, buffer);
        //return &i;

        // DEBT: Do std::forward
        auto i = new item_type(endpoint, time_sent, buffer, args...);
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
        // DEBT: This now means consumer is responsible for GC, which is not thought through
        // at this time
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