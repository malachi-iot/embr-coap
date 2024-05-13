// See README.md
#pragma once

#if defined(ESP_PLATFORM)
#include "esp_log.h"
#endif

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
#if defined(ESP_PLATFORM)
    static constexpr const char* TAG = "retry::Manager";
#endif

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
        void resend(time_point* p, time_point p2);

        typename estd::internal::thisify_function<void(time_point*, time_point)>::
            template model<item_type, &item_type::resend> m;

        item_type(endpoint_type e, time_point t, const_buffer_type&& b, parent_type* parent) :
            base_type(e, t, std::move(b)),
            parent(parent),
            m(this)
        {

        }
    };

    typedef Tracker<time_point, transport_type, item_type> tracker_type;
    //typedef typename tracker_type::item_type item_type;

    typedef embr::internal::scheduler::impl::Function<time_point,
        estd::detail::impl::function_fnptr1> scheduler_impl;

    tracker_type tracker;

    bool on_received(const endpoint_type& endpoint, const const_buffer_type& buffer);

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

        estd::detail::v2::function<void(time_point*, time_point),
            estd::detail::impl::function_fnptr1> f(&i2->m);

        // NOTE: Can only use thisafy and friends when 'this' pointer isn't getting moved around
        scheduler.schedule(due, f);

        // DEBT: Whole semantic vs bitwise confusion necessitates this.  Perhaps we
        // permit a const Pbuf& on the way in to transport?
        const_buffer_type& b = const_cast<const_buffer_type&>(i->buffer());

#if defined(ESP_PLATFORM)
        // mid() is slightly expensive, so only enable this if we really need it
        //ESP_LOGD(TAG, "mid=%x", i->mid());
#endif

        transport().send(b, endpoint);

#if defined(ESP_PLATFORM)
        // DEBT: Filter this further by LwIP - perhaps put this log right into transport itself
        ESP_LOGD(TAG, "sent to/tracking: %s:%u, mid=%x",
            ipaddr_ntoa(endpoint.address()),
            endpoint.port(),
            i->mid());
#endif

        return i;
    }


    template <class TContainer, class TSubject>
    const item_type* send(const endpoint_type& endpoint,
        const_buffer_type&& buffer,
        embr::internal::Scheduler<TContainer, scheduler_impl, TSubject>& scheduler)
    {
        return send(endpoint, clock_type::now(), std::move(buffer), scheduler);
    }

    ESTD_CPP_FORWARDING_CTOR(Manager);
};

}}}}