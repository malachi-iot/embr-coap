// See README.md
#pragma once

#include <estd/cstdint.h>

#include "../../coap/internal/factory.h"
#include "../../coap/internal/constants.h"

#include "../../coap/decoder/streambuf.hpp"
#include "../get_header.h"

namespace embr { namespace coap { namespace experimental { namespace retry {


///
/// \brief Underlying data associated with any potential-retry item
///
struct Metadata
{
    // Lower precision milliseconds
    typedef estd::chrono::duration<uint32_t, estd::milli> milliseconds;

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

    /// Retrieve ms expected to elapse from last send to next retry
    /// @return
    milliseconds delta() const
    {
        // TODO: optimize.  Consider making initial_timeout_ms actually at
        // 10-ms resolution, which should be fully adequate for coap timeouts

        // double initial_timeout_ms with each retransmission
        uint16_t multiplier = 1 << retransmission_counter;

        return milliseconds(initial_timeout_ms * multiplier);
    }

    // DEBT: Need an actual random factor here
    Metadata() : initial_timeout_ms{2500}
    {

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
    // DEBT: Would be nice if timepoint didn't mandate estd::chrono
    typedef TTimePoint time_point;
    typedef TBuffer buffer_type;

// DEBT: Making protected while building things out
//private:
protected:
    const endpoint_type endpoint_;
    buffer_type buffer_;

    // NOTE: Tracking way more than we need here while we hone down architecture
    time_point
        first_transmit_,
        last_transmit_;

    // DEBT: This can likely be optimized into bit struct in parent class
    // DEBT: This does double duty to indicate this class should be deleted/garbage collected.
    // If possible, that would likely be better served as a unique_ptr/shared_ptr
    bool ack_received_;

public:
    Item(const endpoint_type& endpoint, const time_point& time_sent, const buffer_type& buffer) :
        endpoint_{endpoint},
        buffer_{buffer},
        first_transmit_{time_sent},
        ack_received_{false}
    {

    }

    Item(const endpoint_type& endpoint, const time_point& time_sent, buffer_type&& buffer) :
        endpoint_{endpoint},
        buffer_(std::move(buffer)),
        first_transmit_{time_sent},
        ack_received_{false}
    {

    }

    // FIX: This is needed for vector/memory pooling
    // Consider revising estd::vector to not use default constructor as per
    // https://stackoverflow.com/questions/2376989/why-dont-stdvectors-elements-need-a-default-constructor
    Item() : endpoint_{endpoint_type{}}
    {

    }

    Item(const Item& copy_from) = default;
    Item& operator=(const Item& copy_from)
    {
        new (this) Item(copy_from);
        return *this;
    }

    // get MID from sent netbuf, for incoming ACK comparison
    inline uint16_t mid() const
    {
        auto decoder = internal::DecoderFactory<buffer_type>::create(buffer_);

        // FIX: Although this works, it's too sloppy even to be debt.
        // this uses foreknowledge of decoder state machine behavior to
        // do exactly 2 iterations which results in a fully parsed Header
        decoder.process_iterate_streambuf();
        decoder.process_iterate_streambuf();

        Header header = decoder.header_decoder();
        return header.message_id();
    }


    // get Token from sent buffer, for incoming ACK comparison
    // QUESTION: Isn't this only for observable, not CON/ACK?
    coap::layer2::Token token() const
    {
        // TODO: optimize and use header & token decoder only and directly
        auto decoder = internal::DecoderFactory<buffer_type>::create(buffer_);
        coap::layer2::Token token;

        // FIX: Still need to iterate
        decoder.header_decoder();
        decoder.token_decoder();

        //decoder.token(&token);

        return token;
    }

    ESTD_CPP_CONSTEXPR_RET const endpoint_type& endpoint() const
    {
        return endpoint_;
    }

    ESTD_CPP_CONSTEXPR_RET const buffer_type& buffer() const
    {
        return buffer_;
    }

    ESTD_CPP_CONSTEXPR_RET bool ack_received() const
    {
        return ack_received_;
    }

    // DEBT: We'd prefer a friend class or some other less visible way of presenting this
    void ack_received(bool value)
    {
        ack_received_ = value;
    }
};

}}}}