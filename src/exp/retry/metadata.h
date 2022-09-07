// See README.md
#pragma once

#include <estd/cstdint.h>

namespace embr { namespace coap { namespace experimental { namespace retry {


///
/// \brief Underlying data associated with any potential-retry item
///
struct Metadata
{
    // from 0-4 (COAP_MAX_RETRANSMIT)
    // NOTE: Technically at this time this actually represents *queued for transmit* and may
    // or may not reflect whether it has actually been transmitted yet
    uint16_t retransmission_counter : 3;

    // represents in milliseconds initial timeout as described in [1] section 4.2.
    // This is  between COAP_ACK_TIMEOUT and COAP_ACK_TIMEOUT*COAP_ACK_RANDOM_FACTOR
    // which for this variable works out to be 2000-3000
    uint16_t initial_timeout_ms : 12;

    // helper bit to help us avoid decoding outgoing message to see if it's really
    // a CON message.  Or, in otherwords, a cached value indicating outgoing message
    // REALLY is CON.
    // value: 1 = definitely a CON message
    // value: 0 = maybe a CON message, decode required
    uint16_t con_helper_flag_bit : 1;

    // TODO: optimize.  Consider making initial_timeout_ms actually at
    // 10-ms resolution, which should be fully adequate for coap timeouts
    uint32_t delta()
    {
        // double initial_timeout_ms with each retransmission
        uint16_t multiplier = 1 << retransmission_counter;

        return initial_timeout_ms * multiplier;
    }
};

}}}}