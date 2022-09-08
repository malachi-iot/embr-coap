// See README.md
#pragma once

#include <estd/cstdint.h>

#include "factory.h"

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


///
/// @tparam TEndpoint - system specific full address of destination to send retries to
/// @tparam TTimePoint - estd::chrono timepoint
/// @tparam TBuffer - system specific buffer.  For LwIP it may be pbuf or netbuf
template <class TEndpoint, class TTimePoint, class TBuffer>
struct Item : Metadata
{
    typedef TEndpoint endpoint_type;
    typedef TTimePoint timepoint_type;
    typedef TBuffer buffer_type;

private:
    const endpoint_type endpoint_;
    buffer_type buffer_;
    timepoint_type due_;

    bool ack_received_;

public:
    Item(const endpoint_type& endpoint) : endpoint_{endpoint}
    {

    }

    // get MID from sent netbuf, for incoming ACK comparison
    inline uint16_t mid() const
    {
        auto decoder = DecoderFactory<buffer_type>::create(buffer_);
        Header header = decoder.header();
        return header.message_id();
    }


    // get Token from sent buffer, for incoming ACK comparison
    coap::layer2::Token token() const
    {
        // TODO: optimize and use header & token decoder only and directly
        auto decoder = DecoderFactory<buffer_type>::create(buffer_);
        coap::layer2::Token token;

        decoder.header();
        decoder.token(&token);

        return token;
    }
};

}}}}