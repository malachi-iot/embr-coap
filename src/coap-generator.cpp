//
// Created by malachi on 11/13/17.
//

#include "coap-generator.h"

namespace moducom { namespace coap {

bool CoAPGenerator::output_option_iterate(const experimental::layer2::OptionBase &option)
{
    typedef experimental::layer2::OptionGenerator::StateMachine statemachine_t;
    typedef statemachine_t::output_t output_t;

    switch(state)
    {
        case CoAP::Parser::HeaderDone:
            // signals initialization of options phase
            break;
        case CoAP::Parser::Options:
        {
            if (&option != option_state.current_option)
            {
                // reset/advance options parser
            }

            output_t out = option_state.option_state->generate_iterate();
            return out != statemachine_t::signal_continue;
            break;
        }
        case CoAP::Parser::OptionsDone:
            // probably we won't actually know this here
            break;
    }
}

bool CoAPGenerator::output_payload_iterate(const pipeline::MemoryChunk &chunk)
{
    if(state != CoAP::Parser::Payload)
    {
        // FIX: cheap and nasty payload mode init, but it's a start
        payload_state.pos = 0;
        state = CoAP::Parser::Payload;
    }

    size_t advanceable = output.advanceable();

    if(advanceable >= chunk.length)
    {
        // FIX: really we need to pay attention to output.write result
        output.write(chunk.data, chunk.length);
        return true;
    }
    else
    {
        // FIX: really we need to pay attention to output.write result
        output.write(chunk.data, advanceable);
        payload_state.pos += advanceable;
        return false;
    }
    //pipeline::MemoryChunk
}

}}
