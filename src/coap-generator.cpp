//
// Created by malachi on 11/13/17.
//

#include "coap-generator.h"

namespace moducom { namespace coap {

bool CoAPGenerator::output_option_iterate(const experimental::layer2::OptionBase& option)
{
    typedef _option_state_t statemachine_t;
    typedef statemachine_t::output_t output_t;

    statemachine_t& option_state = get_option_state();

    switch(state)
    {
        case CoAP::Parser::HeaderDone:
            // signals initialization of options phase

            // placement new to get around union C++03 limitation
            new (get_option_state_ptr()) statemachine_t(option);

            break;
        case CoAP::Parser::Options:
        {
            if (&option != &option_state.get_option_base())
            {
                option_state.next(option);
                // reset/advance options parser
            }

            output_t out = option_state.generate_iterate();
            return out != statemachine_t::signal_continue;
            break;
        }
        case CoAP::Parser::OptionsDone:
            // probably we won't actually know this here
            // for completeness would like to do a placement delete, but I forgot
            // the syntax
            break;
    }
}

bool CoAPGenerator::output_payload_iterate(const pipeline::MemoryChunk& chunk)
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
