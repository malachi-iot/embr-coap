//
// Created by malachi on 11/13/17.
//

#include "coap-generator.h"

namespace moducom { namespace coap {

typedef size_t (*pipeline_output_helper_fn_t)(moducom::pipeline::MemoryChunk& chunk, void* context);

// FIX:
// Experimental
// Not even close to ready for primetime.  Doesn't handle "in flight" position
// state, and no looping helpers
template <size_t buffer_length>
bool pipeline_output_helper(pipeline::experimental::IBufferProviderPipeline& output,
                            pipeline_output_helper_fn_t pipeline_output_helper_fn,
                            void* context = NULLPTR)
{
    size_t advanceable = output.advanceable();

    if(advanceable == 0) return false;

    pipeline::PipelineMessage message = output.get_buffer(advanceable);

    if(message.length > 0)
    {
        // FIX: this expects advanceable to always exceed needs of pipeline_output_helper_fn
        // but that very well might not be true
        pipeline_output_helper_fn(message, context);
    }
    else
    {
        uint8_t buffer[buffer_length];
        // TODO: optimize, we are wasting PipelineMessage stack at this point
        pipeline::MemoryChunk chunk(buffer, buffer_length);

        pipeline_output_helper_fn(chunk, context);
    }
}


// doing this because pipeline might not be able to take all bytes just yet and
// I only return a true/false instead of number of bytes actually written
bool CoAPGenerator::output_header_iterate()
{
    uint8_t value = header_state.bytes[header_state.pos];

    if(output.write(value))
    {
        if(++header_state.pos == 4) return true;
    }
    return false;
}


bool CoAPGenerator::output_option_next(const experimental::layer2::OptionBase& option)
{
    _option_state_t& option_state = get_option_state();

    option_state.next(option);

    return true;
}


bool CoAPGenerator::output_option_begin(const experimental::layer2::OptionBase& option)
{
    typedef _option_state_t statemachine_t;

    new (get_option_state_ptr()) statemachine_t(option);

    _option_state_t& option_state = get_option_state();

    return true;
}


bool CoAPGenerator::output_option_iterate()
{
    typedef _option_state_t statemachine_t;
    typedef statemachine_t::output_t output_t;

    statemachine_t& option_state = get_option_state();
    const option_t& option = option_state.get_option_base();

    CoAP::ParserDeprecated::SubState sub_state = option_state.state();
    /*
     * TODO: For IBufferProviderPipeline
    size_t advanceable = output.advanceable();

    if(advanceable == 0) return false;

    if(sub_state == substate_t::OptionLengthDone)
    {
        if(option.length < advanceable)
        {
            // TODO: must also check to see if option is a string or opaque type
            // NOTE: this isn't handling zero copy as well as we'd like, this
            // suggests a copy every time for at least the value portion of the
            // option.  Instead, we need a specialized output_option_iterate
            // which only takes length and number (type) and then we manually
            // stuff value through similar to a payload
            //output.write(option.value_opaque, option.length);
        }
    }
    else */
    // signals end of option
    if(sub_state == substate_t::OptionValueDone)
    {
        // signals completion of this particular option to caller
        // probably can be optimized
        return true;
    }

    output_t out = option_state.generate_iterate();

    // TODO: sloppy, clean this code up.  Regardless of option_state machine
    // output, we report back "still processing" until OptionValueDone is
    // reached
    if(out == statemachine_t::signal_continue) return false;

    // copy operation of one character is much preferred to buffer availability
    // juggling
    output.write((uint8_t)out);
    return false;
}

bool CoAPGenerator::output_payload_iterate(const pipeline::MemoryChunk& chunk)
{
    /*
     * TODO: For IBufferProviderPipeline
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
    }*/

    // FIX: Needs testing here
    return output.write(chunk);
    //pipeline::MemoryChunk
}

}}
