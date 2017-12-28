#include <catch.hpp>
#include "../coap-encoder.h"
#include "../mc/memory.h"
#include "../coap_transmission.h"

using namespace moducom::coap;
using namespace moducom::pipeline;

typedef CoAP::OptionExperimental::Numbers number_t;

class ExperimentalPrototypeBlockingOptionEncoder1
{
    typedef moducom::coap::experimental::layer2::OptionGenerator::StateMachine generator_t;
    generator_t generator;

    // option helper
    uint8_t option(uint8_t* output, number_t number, uint16_t length);

    // option helper
    void option(IPipelineWriter& writer, number_t number, uint16_t length);

public:
    ExperimentalPrototypeBlockingOptionEncoder1() : generator(0) {}

    void option(IPipelineWriter& writer, number_t number, const MemoryChunk& value);
    void option(IPipelineWriter& writer, number_t number)
    {
        option(writer, number, 0);
    }
};

class ExperimentalPrototypeBlockingEncoder1
{
private:
    ExperimentalPrototypeBlockingOptionEncoder1 optionEncoder;

    IPipelineWriter& writer;

public:
    ExperimentalPrototypeBlockingEncoder1(IPipelineWriter& writer) : writer(writer) {}

    void header(void* header);

    void token(const MemoryChunk& value);

    void option(number_t number, const MemoryChunk& value)
    {
        optionEncoder.option(writer, number, value);
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

    void payload(const MemoryChunk& value);
};


uint8_t ExperimentalPrototypeBlockingOptionEncoder1::option(uint8_t* output_data, number_t number, uint16_t length)
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
    uint8_t pos = option(buffer, number, length);

    writer.write(buffer, pos); // generated header-ish portions of option, sans value
}


void ExperimentalPrototypeBlockingOptionEncoder1::option(IPipelineWriter& writer, number_t number, const MemoryChunk& value)
{
    option(writer, number, value.length);
    writer.write(value); // value portion
}

TEST_CASE("CoAP encoder tests", "[coap-encoder]")
{
    typedef CoAP::OptionExperimental number_t;

    SECTION("1")
    {
        layer3::MemoryChunk<128> chunk;
        layer3::SimpleBufferedPipelineWriter writer(chunk);
        ExperimentalPrototypeBlockingEncoder1 encoder(writer);

        encoder.option(number_t::ETag, "etag");
        encoder.option(number_t::UriPath, "query");

        REQUIRE(chunk[0] == 0x44);
        REQUIRE(chunk[5] == 0x75);
    }
}