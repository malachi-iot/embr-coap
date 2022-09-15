// See README.md
#pragma once

#include <estd/algorithm.h>
#include <estd/cstdint.h>
#include <estd/forward_list.h>

#include <embr/scheduler.h>

#include "provider.h"
#include "tracker.h"

namespace embr { namespace coap { namespace experimental { namespace retry {



// DEBT: TTransport = TTransportDescriptor
// TODO: Put an instance provider in here to handle TTransport and TTransport&
template <class TClock, class TTransport>
struct Manager : embr::internal::instance_or_reference_provider<TTransport>
{
    typedef embr::internal::instance_or_reference_provider<TTransport> base_type;

    typedef base_type transport_provider;
    typedef typename transport_provider::value_type transport_type;

    typedef typename transport_type::endpoint_type endpoint_type;
    typedef typename transport_type::buffer_type buffer_type;
    typedef typename transport_type::const_buffer_type const_buffer_type;

    typedef TClock clock_type;
    typedef typename clock_type::time_point time_point;

    typedef Item<endpoint_type, time_point, const_buffer_type> item_base;

    transport_type& transport() { return transport_provider::value(); }
    const transport_type& transport() const { return transport_provider::value(); }

    // DEBT: item_type name only temporary.  If we hide the struct then
    // name is permissible
    struct item_type : item_base
    {
        typedef item_base base_type;
        typedef Manager parent_type;

        parent_type* const parent;

        // If ack_received or retransmission counter passes threshold
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
            parent->transport().send(item_base::buffer_, item_base::endpoint());

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
                parent->tracker.untrack(this);
                delete this;
            }
        }

        typename estd::internal::thisify_function<void(time_point*, time_point)>::
            template model<item_type, &item_type::resend> m;

        item_type(endpoint_type e, time_point t, const_buffer_type b, parent_type* parent) :
            base_type(e, t, b),
            parent(parent),
            m(this)
        {

        }
    };

    typedef Tracker<time_point, transport_type, item_type> tracker_type;
    //typedef typename tracker_type::item_type item_type;

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
        if(m == tracker.end()) return false;

        auto v = m.lock();

        v->ack_received(true);

        m.unlock();

        // If we do find a match, then this means we no longer need to look for ACKs because
        // we just got it.
        tracker.untrack(m);
        return true;
    }

    template <class TContainer, class TSubject>
    const item_type* send(const endpoint_type& endpoint,
        time_point time_sent,
        const_buffer_type&& buffer,
        embr::internal::Scheduler<TContainer, scheduler_impl, TSubject>& scheduler)
    {
        const item_type* i = tracker.track(endpoint, time_sent, std::move(buffer), this);
        item_type* i2 = (item_type*) i; // FIX: Kludgey, assign f model requires non-const

        //time_point now = clock_type::now();   // TODO
        time_point due = time_sent + i->delta();

        estd::detail::function<void(time_point*, time_point)> f(&i2->m);

        // NOTE: Can only use thisafy and friends when 'this' pointer isn't getting moved around
        scheduler.schedule(due, f);

        // DEBT: Whole semantic vs bitwise confusion necessitates this.  Perhaps we
        // permit a const Pbuf& on the way in to transport?
        const_buffer_type& b = const_cast<const_buffer_type&>(i->buffer());

        transport().send(b, endpoint);

        return i;
    }

    ESTD_CPP_FORWARDING_CTOR(Manager);
};

}}}}