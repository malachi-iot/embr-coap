//
// Created by Malachi Burke on 11/24/17.
// Encoders spit out encoded bytes based on a more meaningful set of inputs
//

#ifndef MC_COAP_TEST_COAP_ENCODER_H
#define MC_COAP_TEST_COAP_ENCODER_H

#include "coap_transmission.h"
#include "mc/pipeline-writer.h"
#include "coap-token.h"

namespace moducom { namespace coap {

//typedef experimental::layer2::OptionGenerator::StateMachine OptionEncoder;
class OptionEncoder : public experimental::layer2::OptionGenerator::StateMachine
{
public:
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
    void header(pipeline::IPipelineWriter& writer, const CoAP::Header& header)
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
    typedef moducom::coap::experimental::layer2::OptionGenerator::StateMachine generator_t;
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




class BlockingEncoder
{
protected:
    typedef CoAP::ParserDeprecated::State state_t;
    typedef CoAP::ParserDeprecated _state_t;

#ifdef DEBUG
    state_t consistency;

    void state(state_t c) { consistency = c; }
    void assert_state(state_t c)
    {
        ASSERT_ERROR(c, consistency, "State mismatch");
    }
    void assert_not_state(state_t c)
    {
        ASSERT_ERROR(true, c != consistency, "Invalid state");
    }
#else
    void state(state_t) {}
    void assert_state(state_t) {}
    void assert_not_state(state_t) {}

#endif

protected:
    ExperimentalPrototypeBlockingOptionEncoder1 optionEncoder;
    ExperimentalPrototypeBlockingPayloadEncoder1 payloadEncoder;

    pipeline::IPipelineWriter& writer;



public:
    BlockingEncoder(pipeline::IPipelineWriter& writer) :
            consistency(_state_t::Header),
            writer(writer) {}

    void header(const CoAP::Header& header)
    {
        assert_state(_state_t::Header);
        writer.write(header.bytes, 4);
        state(_state_t::HeaderDone);
    }


    void header(CoAP::Header::RequestMethodEnum request_method, CoAP::Header::TypeEnum c = CoAP::Header::Confirmable)
    {
        // initializes with no token and no message id
        CoAP::Header header(c);

        header.code(request_method);

        this->header(header);
    }


    void header(CoAP::Header::Code::Codes response_method, CoAP::Header::TypeEnum c = CoAP::Header::Confirmable)
    {
        CoAP::Header header(c);

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
