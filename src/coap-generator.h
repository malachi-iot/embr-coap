//
// Created by malachi on 11/13/17.
//

#ifndef MC_COAP_TEST_COAP_GENERATOR_H
#define MC_COAP_TEST_COAP_GENERATOR_H

#include "mc/pipeline.h"

namespace moducom { namespace coap {

class CoAPGenerator
{
    CoAP::Parser::State state;
    pipeline::experimental::IBufferProviderPipeline& output;

    struct payload_state_t
    {
        size_t pos;
    };

    struct option_state_t
    {
        experimental::layer2::OptionBase* current_option;
        // FIX: won't be a pointer and needs name improvement
        experimental::layer2::OptionGenerator::StateMachine* option_state;
    };

    union
    {
        payload_state_t payload_state;
        option_state_t option_state;
    };

public:
    CoAPGenerator(pipeline::experimental::IBufferProviderPipeline& output) : output(output) {}

    bool output_option_iterate(const experimental::layer2::OptionBase& option);
    bool output_header_iterate();
    bool output_payload_iterate(const pipeline::MemoryChunk& chunk);
};

}}

#endif //MC_COAP_TEST_COAP_GENERATOR_H
