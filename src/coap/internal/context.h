#pragma once

#include "rfc7641/enum.h"
#include "../context/tags.h"

namespace embr { namespace coap { 

namespace internal {

// DEBT: Poor naming and organization
// This is where utility bitmask goes + auto reply info
class ExtraContext
{
public:
    // For auto response
    estd::layer1::optional<Header::Code::Codes, Header::Code::Empty> response_code;

    struct
    {
        // duplicate MID encountered
        bool dup_mid : 1;
        // payload encountered
        bool payload : 1;
        // true = response has been sent/queued for send, false = response not yet sent
        // DEBT: This may be problematic with stream style (i.e. TCP continuous writes)
        bool response_sent : 1;

        // observe option, if present
        // NOTE: does not retain sequence number
        observable::Options observable : 2;

    }   flags;

    ExtraContext()
    {
        flags.dup_mid = false;
        flags.payload = false;
        flags.response_sent = false;
        flags.observable = observable::option_value_type::null_value();
    }

    observable::option_value_type observe_option() const
    {
        return flags.observable;
    }

    // For any implementors of IncomingContext::reply, they need to call this manually
    // in order for auto reply logic to function
    // DEBT: Kludgey, we'd prefer subject/observe code to do this
    void on_send()
    {
        flags.response_sent = true;
    }
};


}

namespace experimental {


// NOTE: Just an interesting idea at this time
class DecoderContext
{
};


// NOTE: Really would like this (avoid the whole global_encoder nastiness) but
// unclear right now how to incorporate this across different styles of encoder.
// So it remains fully experimental and unused at this time
template <class TEncoder>
class EncoderContext
{
    TEncoder* _encoder;

public:
    TEncoder& encoder() const { return *_encoder; }

    // TODO: signal that encoding is done (probably to datapump, but
    // not necessarily)
    void encoding_complete() {}
};


class OutputContext
{
public:
    template <class TNetBuf>
    void send(TNetBuf& to_send) {}
};



}

}}
