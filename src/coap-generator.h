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

    typedef experimental::layer2::OptionGenerator::StateMachine _option_state_t;

    struct option_state_t
    {
        uint8_t fakebuffer[sizeof(_option_state_t)];
    };

    union
    {
        payload_state_t payload_state;
        option_state_t option_state;
    };

    // workarounds for C++98/C++03 union limitations
    inline _option_state_t* get_option_state_ptr()
    {
        return (_option_state_t*) option_state.fakebuffer;
    }

    _option_state_t& get_option_state()
    {
        return *get_option_state_ptr();
    }

public:
    CoAPGenerator(pipeline::experimental::IBufferProviderPipeline& output) : output(output) {}

    bool output_option_iterate(const experimental::layer2::OptionBase& option);
    bool output_header_iterate();
    bool output_payload_iterate(const pipeline::MemoryChunk& chunk);


    void _output(const experimental::layer2::OptionBase& option)
    {
        while(!output_option_iterate(option))
        {
            // TODO: place a yield statement in here since this is a spinwait
        }
    }


    // this is for payload, we may need a flag (or a distinct call) to designate
    void _output(const pipeline::MemoryChunk& chunk)
    {
        while(!output_payload_iterate(chunk))
        {
            // TODO: place a yield statement in here since this is a spinwait
        }
    }
};

}}

#endif //MC_COAP_TEST_COAP_GENERATOR_H
