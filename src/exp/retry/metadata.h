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
    typedef estd::chrono::duration<uint16_t, estd::milli> milliseconds;

    // from 0-4 (COAP_MAX_RETRANSMIT)
    // NOTE: Technically at this time this actually represents *queued for transmit* and may
    // or may not reflect whether it has actually been transmitted yet
    uint16_t retransmission_counter : 3;

    // represents in milliseconds initial timeout as described in [1] section 4.2.
    // This is  between COAP_ACK_TIMEOUT and COAP_ACK_TIMEOUT*COAP_ACK_RANDOM_FACTOR
    // which for this variable works out to be 2000-3000
    // DEBT: Change this to initial_random_ms since that requires a lower precision,
    // and consider 10ms precision instead of 1ms
    uint16_t initial_timeout_ms : 12;

    // helper bit to help us avoid decoding outgoing message to see if it's really
    // a CON message.  Or, in other words, a cached value indicating outgoing message
    // REALLY is CON.
    // value: 1 = definitely a CON message
    // value: 0 = maybe a CON message, decode required
    // 25MAY24 MB This was useful for when retry was baked deeper into transport and all packets were flowing
    // through it.  Now we curate the packets and track them more manually, so this bit isn't needed
    //uint16_t con_helper_flag_bit : 1;

    // "user" flag, though we likely will be using this always to tag that we got an ACK
    bool flag1 : 1;

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

    /// Have we reached maximum retransmit count? (does not account for ACK)
    bool finished() const
    {
        return retransmission_counter == COAP_MAX_RETRANSMIT;
    }

    // "the initial timeout is set to a random duration [...]
    //  between ACK_TIMEOUT and (ACK_TIMEOUT * ACK_RANDOM_FACTOR)"
    // therefore, initial_random_ms is (ACK_TIMEOUT * ACK_RANDOM_FACTOR) - ACK_TIMEOUT
    static constexpr uint16_t calculate_initial_timeout_ms(uint16_t initial_random_ms)
    {
        return COAP_ACK_TIMEOUT * 1000 + initial_random_ms;
    }

    // DEBT: Need a true default constructor for non-initialized scenarios
    constexpr explicit Metadata(uint16_t initial_random_ms =
        1000 * (COAP_ACK_RANDOM_FACTOR + COAP_ACK_RANDOM_FACTOR_MINIMUM) / 5) :
        retransmission_counter{0},
        initial_timeout_ms{calculate_initial_timeout_ms(initial_random_ms)},
        //con_helper_flag_bit{0}
        flag1{false}
    {

    }
};


///
/// @tparam Endpoint - system specific full address of destination to send retries to
/// @tparam TimePoint - estd::chrono timepoint
/// @tparam Buffer - system specific buffer.  For LwIP it may be pbuf or netbuf
template <class Endpoint, class TimePoint, class Buffer>
struct Item : Metadata
{
    using base_type = Metadata;

    typedef Endpoint endpoint_type;
    // DEBT: Would be nice if timepoint didn't mandate estd::chrono
    typedef TimePoint time_point;
    typedef Buffer buffer_type;

    using duration = typename time_point::duration;

// DEBT: Making protected while building things out
//private:
protected:
    const endpoint_type endpoint_;
    const buffer_type buffer_;

    // NOTE: Tracking way more than we need here while we hone down architecture
    time_point
        first_transmit_;
        //last_transmit_;

    // DEBT: This can likely be optimized into bit struct in parent class
    // DEBT: This does double duty to indicate this class should be deleted/garbage collected.
    // If possible, that would likely be better served as a unique_ptr/shared_ptr
    //bool ack_received_;

public:
    Item(const endpoint_type& endpoint, const time_point& time_sent, milliseconds rand, const buffer_type& buffer) :
        base_type{rand.count()},
        endpoint_{endpoint},
        buffer_{buffer},
        first_transmit_{time_sent}//,
        //ack_received_{false}
    {

    }

    Item(const endpoint_type& endpoint, const time_point& time_sent, milliseconds rand, buffer_type&& buffer) :
        base_type{rand.count()},
        endpoint_{endpoint},
        buffer_(std::move(buffer)),
        first_transmit_{time_sent}//,
        //ack_received_{false}
    {

    }

    Item(const endpoint_type& endpoint, const time_point& time_sent, const buffer_type& buffer) :
        endpoint_{endpoint},
        buffer_{buffer},
        first_transmit_{time_sent}//,
        //ack_received_{false}
    {

    }

    Item(const endpoint_type& endpoint, const time_point& time_sent, buffer_type&& buffer) :
        endpoint_{endpoint},
        buffer_(std::move(buffer)),
        first_transmit_{time_sent}//,
        //ack_received_{false}
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
        // 14MAY24 It's 90% likely we can make a specilized HeaderDecoder<buffer_type>
        // or perhaps an intermediate BufferHelper<buffer_type> which gauruntees a certain
        // amount of contiguous bytes - that would be handy for other scenarios too come
        // to think of it
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
        return flag1;
        //return ack_received_;
    }

    // DEBT: We'd prefer a friend class or some other less visible way of presenting this
    void ack_received(bool value)
    {
        //ack_received_ = value;
        flag1 = value;
    }

    time_point first_transmit() const { return first_transmit_; }
};

}}}}
