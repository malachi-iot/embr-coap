#pragma once

#include "../../option.h"
#include "decode.h"

// Experimental primarily due to naming
// from https://tools.ietf.org/html/rfc7959#section-2.1
namespace embr { namespace coap { namespace experimental {

// Remember:
// block1 in request is descriptive
// block2 in response is descriptive
// block2 in request is negotiative
// block1 in response is a kind of ACK

// DEBT: For the moment, this is hard wired as a server role.  In particular:
// - CON requests are received, not sent
// - ACK responses are sent, not received
// See https://datatracker.ietf.org/doc/html/rfc7959#section-3.2

class BlockwiseStateMachine
{
    using buffer_type = estd::span<const uint8_t>;

    enum States
    {
        STATE_IDLE,
        STATE_REQUEST_RECEIVING,
        STATE_REQUEST_RECEIVED,
    };

    States state_;

public:
    template <typename Int>
    struct Transfer
    {
        using value_type = Int;

        Int total_size_;
        Int current_block_ : (sizeof(Int) * 8) - 4;
        bool more_ : 1;
        Int szx_ : 3;

        Int block_size() const { return 1 << szx_; }
    };

    using transfer_type = Transfer<unsigned>;

private:
    transfer_type request_, response_;

public:
    const transfer_type& request() const { return request_; }
    void option_received(Option::Numbers number, const buffer_type& value)
    {
        switch(number)
        {
            case Option::Block1:
                request_.szx_ = option_block_decode_szx(value);
                request_.more_ = option_block_decode_m(value);
                request_.current_block_ = option_block_decode_num(value);
                break;

            case Option::Size1:
                request_.total_size_ = UInt::get<unsigned>(value);
                break;

            default:    break;
        }
    }
};

}}}