#include <catch.hpp>
#include "../coap-encoder.h"
#include "../mc/memory.h"
#include "../coap_transmission.h"

using namespace moducom::coap;
using namespace moducom::pipeline;

typedef CoAP::OptionExperimental::Numbers number_t;

// NOTE: Too dumb to live
class ExperimentalPrototypeBlockingHeaderEncoder1
{
public:
    void header(IPipelineWriter& writer, const CoAP::Header& header)
    {
        writer.write(header.bytes, 4);
    }
};


// NOTE: Too dumb to live
class ExperimentalPrototypeBlockingTokenEncoder1
{
public:
    void token(IPipelineWriter& writer, const MemoryChunk& value)
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
    uint8_t _option(uint8_t* output, number_t number, uint16_t length);

    ExperimentalPrototypeOptionEncoder1() : generator(0) {}
};


class ExperimentalPrototypeNonBlockingOptionEncoder1 : public ExperimentalPrototypeOptionEncoder1
{
    uint8_t buffer[8];

protected:
    bool option(IBufferedPipelineWriter& writer, number_t number, const MemoryChunk& value);
};


class ExperimentalPrototypeBlockingOptionEncoder1 : public ExperimentalPrototypeOptionEncoder1
{
    // option helper
    void option(IPipelineWriter& writer, number_t number, uint16_t length);

public:

    void option(IPipelineWriter& writer, number_t number, const MemoryChunk& value);
    void option(IPipelineWriter& writer, number_t number)
    {
        option(writer, number, 0);
    }
};


class ExperimentalPrototypeBlockingPayloadEncoder1
{
    bool marker_written;

public:
    ExperimentalPrototypeBlockingPayloadEncoder1() : marker_written(false) {}

    void payload(IPipelineWriter& writer, const MemoryChunk& chunk);
};


class ExperimentalPrototypeBlockingEncoder1
{
private:
    typedef CoAP::Parser::State state_t;
    typedef CoAP::Parser _state_t;

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

    ExperimentalPrototypeBlockingOptionEncoder1 optionEncoder;
    ExperimentalPrototypeBlockingPayloadEncoder1 payloadEncoder;

    IPipelineWriter& writer;



public:
    ExperimentalPrototypeBlockingEncoder1(IPipelineWriter& writer) :
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

    void token(const MemoryChunk& value)
    {
        assert_state(_state_t::HeaderDone);
        writer.write(value);
        state(_state_t::TokenDone);
    }

    void option(number_t number, const MemoryChunk& value)
    {
        assert_not_state(_state_t::Header);
        optionEncoder.option(writer, number, value);
        state(_state_t::Options);
    }

    void option(number_t number)
    {
        optionEncoder.option(writer, number);
    }

    void option(number_t number, const char* str)
    {
        MemoryChunk chunk((uint8_t*) str, strlen(str));
        option(number, chunk);
    }

    void payload(const MemoryChunk& value)
    {
        payloadEncoder.payload(writer, value);
    }

    void payload(const char* str)
    {
        payload(MemoryChunk((uint8_t*)str, strlen(str)));
    }
};


uint8_t ExperimentalPrototypeOptionEncoder1::_option(uint8_t* output_data, number_t number, uint16_t length)
{
    moducom::coap::experimental::layer2::OptionBase optionBase(number);

    optionBase.length = length;

    generator.next(optionBase);

    uint8_t pos = 0;

    // OptionValueDone is for non-value version benefit (always reached, whether value is present or not)
    // OptionValue is for value version benefit, as we manually handle value output
    CoAP::Parser::SubState isDone = length > 0 ? CoAP::Parser::OptionValue : CoAP::Parser::OptionValueDone;

    while (generator.state() != isDone)
    {
        generator_t::output_t output = generator.generate_iterate();

        if (output != generator_t::signal_continue)
            output_data[pos++] = output;
    }

    return pos;
}


void ExperimentalPrototypeBlockingOptionEncoder1::option(IPipelineWriter& writer, number_t number, uint16_t length)
{
    uint8_t buffer[8];
    // NOTE: Needs underscore on _option as overloading logic can't resolve uint8* vs IPipelineWriter& for some reason
    uint8_t pos = _option(buffer, number, length);

    writer.write(buffer, pos); // generated header-ish portions of option, sans value
}


void ExperimentalPrototypeBlockingOptionEncoder1::option(IPipelineWriter& writer, number_t number, const MemoryChunk& value)
{
    option(writer, number, value.length);
    writer.write(value); // value portion
}


bool ExperimentalPrototypeNonBlockingOptionEncoder1::option(IBufferedPipelineWriter& writer, number_t number,
                                                            const MemoryChunk& value)
{
    // Need mini-state machine to determine if we are writing value chunk or
    // pre-value chunk
    if(generator.state() == CoAP::Parser::OptionSize)
    {
        _option(buffer, number, value.length);
    }

    // TODO: examine writable to acquire non-blocking count.  Not to be confused with the very similar buffered stuff
    // writable() doesn't exist yet, but should be a part of the base IPipelineWriter - which is looking more and more
    // like a stream all the time
    // NOTE: Probably should draw a distinction between Reader/Writer and PipelineReader/PipelineWriter.  remember,
    // original purpose of pipeline writes were to carry the memory descriptors themselves around.  It seems I've strayed
    // from that due to all the pieces floating around here.  Maybe we need it fully broken out:
    // Reader, BufferedReader, PipelineReader, BufferedPipelineReader
    //writer.writable();
}

void ExperimentalPrototypeBlockingPayloadEncoder1::payload(IPipelineWriter& writer, const MemoryChunk& chunk)
{
    if(!marker_written)
    {
        writer.write(0xFF);
        marker_written = true;
    }

    writer.write(chunk);
}

TEST_CASE("CoAP encoder tests", "[coap-encoder]")
{
    typedef CoAP::OptionExperimental number_t;

    SECTION("1")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter writer(chunk);
        ExperimentalPrototypeBlockingEncoder1 encoder(writer);

        encoder.header(CoAP::Header::Get);

        encoder.option(number_t::ETag, "etag");
        encoder.option(number_t::UriPath, "query");

        encoder.payload("A payload");

        const int option_pos = 4;

        REQUIRE(chunk[option_pos + 0] == 0x44);
        REQUIRE(chunk[option_pos + 5] == 0x75);
        REQUIRE(chunk[option_pos + 6] == 'q');
    }
}