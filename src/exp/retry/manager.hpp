#pragma once

#include "manager.h"
#include "tracker.hpp"

namespace embr { namespace coap { namespace experimental { namespace retry {

template <class TClock, class TTransport>
void Manager<TClock, TTransport>::item_type::resend(time_point* p, time_point p2)
{
    // DEBT: 'delete this' inside here works, but is easy to get wrong.
    // Specifically, functor scheduler invokes this and as long as 'p'
    // remains unchanged, it is considered a one shot event.  Therefore,
    // no further activity happens and it is safe to delete 'this'

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



/// Evaluates whether this is an ACK for something we're tracking.
/// @param endpoint
/// @param buffer
/// @return true if we this is an ACK and we have now untracked it
template <class TClock, class TTransport>
bool Manager<TClock, TTransport>::on_received(const endpoint_type& endpoint, const const_buffer_type& buffer)
{
    //auto h = DecoderFactory<const_buffer_type>::header(buffer);
    Header h = get_header(buffer);

    return untrack_if_ack(tracker, h, endpoint);
}


}}}}