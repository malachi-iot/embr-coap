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

    typedef Tracker<time_point, transport_type> tracker_type;
    typedef typename tracker_type::item_type item_type;

    typedef embr::internal::scheduler::impl::Function<time_point> scheduler_impl;


    tracker_type tracker;

    /// Evaluates whether this is an ACK for something we're tracking.
    /// @param endpoint
    /// @param buffer
    /// @return true if we this is an ACK and we have now untracked it
    bool on_received(const endpoint_type& endpoint, const const_buffer_type& buffer)
    {
        //auto h = DecoderFactory<const_buffer_type>::header(buffer);
        Header h = get_header(buffer);

        // We only look for ACK here
        if(h.type() != header::Types::Acknowledgement)
            return false;

        // Once we establish it's an ACK, then evaluated tracked connections to see if
        // a matching MID and endpoint are out there
        typename tracker_type::iterator m = tracker.match(endpoint, h.message_id());

        // If we can't find one, then stop here and indicate nothing found
        if(m == tracker.tracked.end()) return false;

        auto v = m.lock();

        v->ack_received(true);

        m.unlock();

        // If we do find a match, then this means we no longer need to look for ACKs because
        // we just got it.
        tracker.untrack(m);
        return true;
    }

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

        transport_type::send(endpoint, buffer);

        return i;
    }
};

}}}}