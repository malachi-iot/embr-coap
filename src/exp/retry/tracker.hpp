#pragma once

#include "tracker.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

// NOTE: Tracker isn't actually CoAP specific, but this helper function is
template <class TTimePoint, class TTransport, class TItem>
bool untrack_if_ack(
    Tracker<TTimePoint, TTransport, TItem>& tracker,
    Header h,
    const typename TTransport::endpoint_type& endpoint)
{
    typedef Tracker<TTimePoint, TTransport, TItem> tracker_type;

#if defined(ESP_PLATFORM)
    static const char* const TAG = "untrack_if_ack";
#endif

    // We only look for ACK here
    if(h.type() != header::Types::Acknowledgement)
    {
#if defined(ESP_PLATFORM)
        ESP_LOGD(TAG, "on_received: aborting since type is not ACK");
#endif
        return false;
    }

    const uint16_t mid = h.message_id();

    // Once we establish it's an ACK, then evaluated tracked connections to see if
    // a matching MID and endpoint are out there
    typename tracker_type::iterator m = tracker.match(endpoint, mid);

    // If we can't find one, then stop here and indicate nothing found
    if(m == tracker.end())
    {
#if defined(ESP_PLATFORM)
        // DEBT: Assumes LwIP
        ESP_LOGD(TAG, "on_received: aborting since endpoint %s:%u and mid %x were not matched",
            ipaddr_ntoa(endpoint.address()),
            endpoint.port(),
            mid);
#endif
        return false;
    }

    auto v = m.lock();

    v->ack_received(true);

    m.unlock();

    // If we do find a match, then this means we no longer need to look for ACKs because
    // we just got it.
    tracker.untrack(m);
    return true;
}


}}}}