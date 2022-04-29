//
// Created by Malachi Burke on 11/24/17.
// Encoders spit out encoded bytes based on a more meaningful set of inputs
//

#ifndef MC_COAP_TEST_COAP_ENCODER_H
#define MC_COAP_TEST_COAP_ENCODER_H

#include "coap_transmission.h"
#include "coap/token.h"
#include "coap/decoder.h"

namespace embr { namespace coap {

// TODO: Split this out into OptionEncoderBase,
// OptionEncoder and OptionEncoderWithValue, the latter being the rarified case
// in which we wish to actually push the value through the encoder, vs. merely
// skipping past it
class OptionEncoder : public Option,
    public StateHelper<Option::State>
{
    typedef experimental::layer2::OptionBase option_base_t;
public:
    typedef State state_t;
    typedef Option _state_t;

    typedef int16_t output_t;

    static CONSTEXPR output_t signal_continue = -1;

    uint16_t current_option_number;
    // FIX: pos probably needs to be a uint16_t unless we eschew
    // value-processing OR accept a hard limit of 255 bytes
    uint8_t pos;
    const option_base_t* option_base;

    void initialize();

public:
    OptionEncoder() { initialize(); }

    OptionEncoder(const option_base_t& option_base) :
            option_base(&option_base)
    {
        initialize();
    }

    const option_base_t& get_option_base() const
    {
        return *option_base;
    }

    output_t generate_iterate();
    uint8_t generate()
    {
        output_t result;
        do
        {
            result = generate_iterate();
        } while(result == signal_continue);

        return result;
    }

    // do really do this might have to use my fancy FRAB in-place init
    // helper, but then we'd be losing "current_option_number" so
    // really we probably need to un-isolate this generator behavior
    // so that it may start up in a less initialized state, and be
    // able to retain its state between options, or, in other words,
    // turn this into a multi-option capable state machine
    // TODO: make this option_base 'const' so that nothing can
    // modify contents of option_base itself (having trouble
    // figuring out syntax for that)
    /*
    void reset(const OptionBase& option)
    {
        option_base = option;
        pos = 0;
    } */

    void next()
    {
        // specifically leaves option_number alone
        state(_state_t::FirstByte);

    }

    void next(const option_base_t& option)
    {
        next();

        // pos gets reset by state machine, so leave it out
        //pos = 0;
        option_base = &option;
    }


    // for scenarios when we had to leave off option processing and our option_base when
    // out of scope
    void resume(const option_base_t& option)
    {
        option_base = &option;
    }


    // for scenarios when we have to leave off option processing, usually because we
    // temporarily ran out of input or output buffer
    void pause()
    {
        option_base = NULLPTR;
    }

    void reset()
    {
        initialize();
    }


    // use this to specifically fast forward the state machine past the value bytes
    void fast_forward()
    {
        ASSERT_ERROR(OptionValue, state(), "Can only call fast forward during option value processing");

        state(OptionDone);
    }
};

namespace experimental {

class EncoderBase
{
protected:
    typedef internal::root_state_t state_t;
    typedef internal::_root_state_t _state_t;

#ifdef DEBUG
    state_t consistency;

    void state(state_t c) { consistency = c; }
    state_t state() const { return consistency; }

    void assert_state(state_t c)
    {
        ASSERT_ERROR(c, consistency, "State mismatch");
    }
    void assert_not_state(state_t c)
    {
        ASSERT_ERROR(true, c != consistency, "Invalid state");
    }

    EncoderBase() :
        consistency(_state_t::Header) {}
#else
    void state(state_t) {}
    void assert_state(state_t) {}
    void assert_not_state(state_t) {}

#endif

};

}


}}

#endif //MC_COAP_TEST_COAP_ENCODER_H
