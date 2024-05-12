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
        STATE_RESPONSE_SENDING,
        STATE_RESPONSE_SENT,

        STATE_REQUEST_SENDING,
        STATE_REQUEST_SENT,
        STATE_RESPONSE_RECEIVING,
        STATE_RESPONSE_RECEIVED,
    };

    States state_ = STATE_IDLE;

public:
    // DEBT: Combine this with OptionBlock
    template <typename Int>
    struct Transfer
    {
        using value_type = Int;

        Option::ContentFormats content_format_;
        Int total_size_;
        Int current_block_ : (sizeof(Int) * 8) - 4;
        bool more_ : 1;
        Int szx_ : 3;

        Int block_size() const { return 1 << (szx_ + 4); }
        void block_size(Int size)
        {
            unsigned v = std::log2(size);
            szx_ = v - 4;
        }

        // In context of REQUEST_RECEIVING, help determine if final payload size is correct
        Int remaining() const
        {
            return total_size_ - (current_block_ * block_size());
        }

        uint8_t encode(uint8_t* output) const
        {
            return option_block_encode(output, current_block_, more_, szx_);
        }
    };

    using transfer_type = Transfer<unsigned>;

private:
    transfer_type request_, response_;

public:
    const transfer_type& request() const { return request_; }

    void initiate_send(unsigned total_size, Option::ContentFormats format)
    {
        response_.content_format_ = format;
        response_.total_size_ = total_size;
        response_.current_block_ = 0;
        response_.szx_ = 3;     // 128 , arbitrary default
    }

    void option_received(Option::Numbers number, const buffer_type& value)
    {
        switch(number)
        {
            case Option::ContentFormat:
            {
                auto v = (Option::ContentFormats) UInt::get<unsigned>(value);

                if(state_ == STATE_IDLE)
                    request_.content_format_ = v;
                else
                {
                    if(request_.content_format_ != v)
                    {
                        // TODO: return a 408
                    }
                }
                break;
            }

            case Option::Block1:
                if(state_ == STATE_IDLE)
                    state_ = STATE_REQUEST_RECEIVING;

                request_.szx_ = option_block_decode_szx(value);
                request_.more_ = option_block_decode_m(value);
                request_.current_block_ = option_block_decode_num(value);

                if(request_.more_ == 0)
                    state_ = STATE_REQUEST_RECEIVED;

                break;

            case Option::Size1:
                request_.total_size_ = UInt::get<unsigned>(value);
                break;

            default:    break;
        }
    }

    template <class Encoder>
    void encode_block1(Encoder&) {}

    template <class Encoder>
    void encode_block2(Encoder& encoder)
    {
        uint8_t value[4];
        uint16_t length = response_.encode(value);
        encoder.option_raw(Option::Block2, length);
        encoder.rdbuf()->sputn((const char*)value, length);
    }

    template <class Encoder>
    void encode_size1(Encoder&) {}

    template <class Encoder>
    void encode_size2(Encoder& encoder)
    {
        encoder.option(Option::Size2, response_.total_size_);
    }

    template <class Encoder>
    void encode_options(Encoder& encoder)
    {
        encode_block2(encoder);
        encode_size2(encoder);
    }
};

}}}