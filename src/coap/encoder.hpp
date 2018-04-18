#include "encoder.h"

namespace moducom { namespace coap {

template <class TNetBuf>
bool NetBufEncoder<TNetBuf>::option_header(option_number_t number, uint16_t value_length)
{
    assert_not_state(_state_t::Header);
    assert_not_state(_state_t::Token);
    assert_not_state(_state_t::Payload);

    typedef OptionEncoder oe_t;
    typedef experimental::layer2::OptionBase option_base_t;

    option_base_t ob(number);

    oe_t& oe = option_encoder;

    // utilized mainly to popular "Option Length" (for value) portion, actual value
    // itself is serviced outside option_header
    ob.length =  value_length;

    // OptionValueDone is for non-value version benefit (always reached, whether value is present or not)
    // OptionValue is for value version benefit, as we manually handle value output
    Option::State isDone = value_length > 0 ? Option::OptionValue : Option::OptionValueDone;

    if(oe.state() == isDone || oe.state() == Option::FirstByte)
    {
        // This should work even on partial calls to option_header *provided* number and value_length
        // remain constant across calls.  If they do not, behavior is undefined
        oe.next(ob);
    }
    else
    {
        // FIX: need to reassign option base here.  May get a false positive of operational
        // because stack frame might be identical when we get here, but don't be lazy and
        // let that slide
    }

    uint8_t* output_data = data();
    int pos = 0;

    // while option encoder is not done, keep spitting out to netbuf
    while (oe.state() != isDone && pos < size())
    {
        oe_t::output_t output = oe.generate_iterate();

        // remember signal_continue means further processing is needed before
        // we can spit out a byte - often used to give consumers an opportunity
        // to inspect state
        if (output != oe_t::signal_continue)
            output_data[pos++] = output;
    }

    advance(pos);

    state(_state_t::Options);

    bool retval = oe.state() == isDone;

    return retval;
}


template <class TNetBuf>
bool NetBufEncoder<TNetBuf>::option(option_number_t number, const pipeline::MemoryChunk& option_value, bool last_chunk)
{
    // NOTE: last_chunk not yet supported

    const uint16_t len = option_value.length();

    if(!option_header(number, len)) return false;

    size_type written = write(option_value.data(), len);

    // TODO: resolve who tracks partial written position.  Perhaps inbuild OptionDecoder's pos is
    // a good candidate, though presently it's only an 8-bit value since it's oriented towards header
    // output tracking only
    bool done = written == len;

    // if this is the last_chunk *and* we have written all the value bytes
    if(last_chunk && done)
    {
        // then indicate to option encoder it's time to start over (we're skipping option_encoder's value
        // processor in this case.  Might be better to have a specialized call for that)

        // FIX: reset vs initialize vs next a bit confusing, simplify
        option_encoder.next();
    }
    else
    {
    }

    return done;
}

template <class TNetBuf>
template <class TString>
bool NetBufEncoder<TNetBuf>::option(option_number_t number, TString s)
{
    const uint16_t len = s.length();

    if(!option_header(number, len)) return false;

    // return true if we copied all the string bytes
    // return false if we copied less than all the string bytes
    return write(s) == s.length();
}

template <class TNetBuf>
template <class TString>
bool NetBufEncoder<TNetBuf>::payload(TString s)
{
    if(!payload_header()) return false;

    // return true if we copied all the string bytes
    // return false if we copied less than all the string bytes
    return write(s) == s.length();
}

}}
