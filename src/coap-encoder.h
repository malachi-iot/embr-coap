//
// Created by Malachi Burke on 11/24/17.
// Encoders spit out encoded bytes based on a more meaningful set of inputs
//

#ifndef MC_COAP_TEST_COAP_ENCODER_H
#define MC_COAP_TEST_COAP_ENCODER_H

#include "coap_transmission.h"
#include "mc/pipeline-writer.h"
#include "coap-token.h"
#include "coap-decoder.h"

namespace moducom { namespace coap {

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

        state(OptionValueDone);
    }

    // deprectated
    bool process_iterate(pipeline::IBufferedPipelineWriter& writer);
};

namespace experimental {

class ExperimentalSessionContext
{
    moducom::coap::layer2::Token token;
};

// NOTE: Too dumb to live
class ExperimentalPrototypeBlockingHeaderEncoder1
{
public:
    void header(pipeline::IPipelineWriter& writer, const Header& header)
    {
        writer.write(header.bytes, 4);
    }
};


// NOTE: Too dumb to live
class ExperimentalPrototypeBlockingTokenEncoder1
{
public:
    void token(pipeline::IPipelineWriter& writer, const pipeline::MemoryChunk& value)
    {
        writer.write(value);
    }
};


class ExperimentalPrototypeOptionEncoder1
{
protected:
    //typedef moducom::coap::experimental::layer2::OptionGenerator::StateMachine generator_t;
    typedef OptionEncoder generator_t;

    generator_t generator;

    // option helper, fills output with non-value portion of option
    uint8_t _option(uint8_t* output, option_number_t number, uint16_t length);

    ExperimentalPrototypeOptionEncoder1() : generator(0) {}
};


class ExperimentalPrototypeNonBlockingOptionEncoder1 : public ExperimentalPrototypeOptionEncoder1
{
    uint8_t buffer[8];

protected:
    bool option(pipeline::IBufferedPipelineWriter& writer, option_number_t number, const pipeline::MemoryChunk& value);
};


class ExperimentalPrototypeBlockingOptionEncoder1 : public ExperimentalPrototypeOptionEncoder1
{
public:

    // FIX: Only public so that experimental BufferedEncoder can use it
    // option helper.  Either completely writes a 0 length value option, or
    // prepares for a write of a > 0 length value option
    void option(pipeline::IPipelineWriter& writer, option_number_t number, uint16_t length);

public:

    void option(pipeline::IPipelineWriter& writer, option_number_t number, const pipeline::MemoryChunk& value);
    void option(pipeline::IPipelineWriter& writer, option_number_t number)
    {
        option(writer, number, 0);
    }
};


class ExperimentalPrototypeBlockingPayloadEncoder1
{
    bool marker_written;

public:
    ExperimentalPrototypeBlockingPayloadEncoder1() : marker_written(false) {}

    void payload(pipeline::IPipelineWriter& writer, const pipeline::MemoryChunk& chunk);
};



class EncoderBase
{
protected:
    typedef experimental::root_state_t state_t;
    typedef experimental::_root_state_t _state_t;

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

class BlockingEncoder : public EncoderBase
{

protected:
    ExperimentalPrototypeBlockingOptionEncoder1 optionEncoder;
    ExperimentalPrototypeBlockingPayloadEncoder1 payloadEncoder;

    pipeline::IPipelineWriter& writer;

public:
    BlockingEncoder(pipeline::IPipelineWriter& writer) :
            writer(writer) {}

    void header(const Header& header)
    {
        assert_state(_state_t::Header);
        writer.write(header.bytes, 4);
        state(_state_t::HeaderDone);
    }


    void header(Header::RequestMethodEnum request_method, Header::TypeEnum c = Header::Confirmable)
    {
        // initializes with no token and no message id
        Header header(c);

        header.code(request_method);

        this->header(header);
    }


    void header(Header::Code::Codes response_method, Header::TypeEnum c = Header::Confirmable)
    {
        Header header(c);

        header.response_code(response_method);

        this->header(header);
    }


    // remember that, for now, we have to explicitly set token length in header too
    void token(const pipeline::MemoryChunk& value)
    {
        assert_state(_state_t::HeaderDone);
        writer.write(value);
        state(_state_t::TokenDone);
    }


    void token(const moducom::coap::layer2::Token& value)
    {
        assert_state(_state_t::HeaderDone);
        writer.write(value.data(), value.length());
        state(_state_t::TokenDone);
    }

    void option(option_number_t number, const pipeline::MemoryChunk& value)
    {
        assert_not_state(_state_t::Header);
        optionEncoder.option(writer, number, value);
        state(_state_t::Options);
    }

    void option(option_number_t number)
    {
        optionEncoder.option(writer, number);
    }

    void option(option_number_t number, const char* str)
    {
        pipeline::MemoryChunk chunk((uint8_t*) str, strlen(str));
        option(number, chunk);
    }

    void payload(const pipeline::MemoryChunk& value)
    {
        payloadEncoder.payload(writer, value);
    }

    void payload(const char* str)
    {
        payload(pipeline::MemoryChunk((uint8_t*)str, strlen(str)));
    }
};


}


}}

#endif //MC_COAP_TEST_COAP_ENCODER_H
