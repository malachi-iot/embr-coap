//
// Created by malachi on 10/29/17.
//

#include <catch.hpp>

#include "../coap_daemon.h"
#include "../coap-responder.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::pipeline;


class TestResponder2 : public GeneratorResponder
{
public:
    TestResponder2(IPipeline& output) : GeneratorResponder(output) {}


    virtual void OnOption(const CoAP::OptionExperimentalDeprecated& option);
    virtual void OnPayload(const uint8_t message[], size_t length);

    // spit out options and payload
    void process();
};

void TestResponder2::OnOption(const CoAP::OptionExperimentalDeprecated& option)
{

}


void TestResponder2::OnPayload(const uint8_t* message, size_t length)
{

}


void TestResponder2::process()
{
    // remember we have to spit them out in order

    //option_t option(option_number_t::UriPath);
    option_t option(11);

    option.value_string = "TESTPATH";
    option.length = 8;

    generator.output_option_begin(option);
    generator.output_option();

    option.value_string = "TESTPAT2";
    generator.output_option_next(option);
    generator.output_option();
}


TEST_CASE("CoAP daemon tests", "[coap-daemon]")
{
    moducom::pipeline::MemoryChunk coap_chunk(buffer_16bit_delta, sizeof(buffer_16bit_delta));

#ifndef CLEANUP_TRANSMISSION_CPP
    SECTION("Pipeline Daemon")
    {
        BasicPipeline _net_in, net_out;
        // Not sure why this was necessary?
        // visibility on method inheritance affected somehow?
        IPipeline& net_in = _net_in;

        PipelineDaemon daemon(net_in, net_out);

        net_in.write(coap_chunk);

        daemon.process_iterate();
    }
#endif
    SECTION("Generator Responder")
    {
        static uint8_t output[] =
        {
                0x40, 0x00, 0x00, 0x00, // 4: fully blank header
                0xB8, // option 11, length 8
                'T', 'E', 'S', 'T', 'P', 'A', 'T', 'H',
                0x08, // stay on option 11, another length 8
                'T', 'E', 'S', 'T', 'P', 'A', 'T', '2',
        };
        moducom::pipeline::layer3::MemoryChunk<1024> buffer_out;

        layer3::SimpleBufferedPipeline net_out(buffer_out);
        BasicPipeline net_in;
        TestResponder2 responder(net_out);
        CoAP::ParseToIResponderDeprecated parseToResponder(&responder);

        parseToResponder.process(buffer_16bit_delta, sizeof(buffer_16bit_delta));
        while(!responder.process_header_and_token_iterate());
        responder.process();

        for(int i = 0; i < sizeof(output); i++)
        {
            INFO("i = " << i);
            REQUIRE(buffer_out.data[i] == output[i]);
        }

        // Just so I can observe stuff in debugger
        REQUIRE(1 == 1);
    }
}