#pragma once

#include "manager.h"

namespace embr { namespace coap { namespace experimental { namespace retry {

/// Evaluates whether this is an ACK for something we're tracking.
/// @param endpoint
/// @param buffer
/// @return true if we this is an ACK and we have now untracked it
template <class TClock, class TTransport>
bool Manager<TClock, TTransport>::on_received(const endpoint_type& endpoint, const const_buffer_type& buffer)
{
    //auto h = DecoderFactory<const_buffer_type>::header(buffer);
    Header h = get_header(buffer);

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