//
// Created by Malachi Burke on 11/24/17.
//

#include "coap-encoder.h"

namespace moducom { namespace coap {



void OptionEncoder::initialize()
{
    state(FirstByte);
    current_option_number = 0;
    pos = 0;
}


uint8_t generator_helper(uint16_t value, int pos = 0)
{
    if (value < experimental::_extended_mode_t::Extended8Bit)
    {
        return value;
    }
    else if (value < COAP_EXTENDED8_BIT_MAX)
    {
        if (pos == 0) return experimental::_extended_mode_t::Extended8Bit;

        value -= 13;

        if (pos == 1) return value;
    }
    else if (value < COAP_EXTENDED16_BIT_MAX)
    {
        if (pos == 0) return experimental::_extended_mode_t::Extended16Bit;

        value -= 269;

        if (pos == 1) return (value >> 8);
        if (pos == 2) return (value & 0xFF);
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}

// TODO: make all these ins and outs a different type
// so that we can spit out -1 or similar indicating "more processing required"
// so that we don't have to pull stunts to pop around extra steps
OptionEncoder::output_t OptionEncoder::generate_iterate()
{
    const option_base_t& option_base = *this->option_base;
    uint8_t option_delta_root = generator_helper(option_base.number - current_option_number);
    uint8_t option_length_root = generator_helper(option_base.length);

    switch (state())
    {
        case _state_t::FirstByte:
            pos = 0;
            state(_state_t::FirstByteDone);

            option_length_root |= option_delta_root << 4;

            return option_length_root;

        case _state_t::FirstByteDone:
            if (option_delta_root >= _state_t::Extended8Bit)
                state(_state_t::OptionDelta);
            else if (option_length_root >= _state_t::Extended8Bit)
                state(_state_t::OptionDeltaDone);
            else
                state(_state_t::OptionDeltaAndLengthDone);

            return signal_continue;

        case _state_t::OptionDelta:
        {
            uint8_t option_delta_next = generator_helper(option_base.number - current_option_number, ++pos);
            if (option_delta_root == _state_t::Extended8Bit)
                state(_state_t::OptionDeltaDone);
            else if (option_delta_root == _state_t::Extended16Bit && pos == 2)
                state(_state_t::OptionDeltaDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_delta_next;
        }

        case _state_t::OptionDeltaDone:
            current_option_number = option_base.number;
            if (option_length_root >= _state_t::Extended8Bit)
                state(_state_t::OptionLength);
            else
                state(_state_t::OptionLengthDone);

            return signal_continue;

        case _state_t::OptionLength:
        {
            uint8_t option_length_next = generator_helper(option_base.length, ++pos);
            if (option_length_next == _state_t::Extended8Bit)
                state(_state_t::OptionLengthDone);
            else if (option_length_next == _state_t::Extended16Bit && pos == 2)
                state(_state_t::OptionLengthDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_length_next;
        }

        case _state_t::OptionDeltaAndLengthDone:
            current_option_number = option_base.number;

        case _state_t::OptionLengthDone:
            if (option_base.length == 0)
            {
                state(_state_t::OptionValueDone);
            }
            else
            {
                pos = 0;
                state(_state_t::OptionValue);
            }

            return signal_continue;

        case _state_t::OptionValue:
            // TODO: Document why we're doing length - 1 here
            if (pos == option_base.length - 1)
                state(_state_t::OptionValueDone);

            return option_base.value_opaque[pos++];

        case _state_t::OptionValueDone:
            // technically it's more like a signal_done but until a new option
            // is loaded in, it's reasonable for state machine to iterate forever
            // on OptionValueDone state
            return this->signal_continue;

        // because some compilers hate unattended cases
        // Note we should never reach here, lingering cases are
        // ValueStart and Payload
        default: break;
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}

bool OptionEncoder::process_iterate(pipeline::IBufferedPipelineWriter& writer)
{
    pipeline::PipelineMessage output = writer.peek_write();

    output_t value;
    size_t length = 0;
    size_t _length = output.length();

    while((value = generate_iterate() != signal_continue) && _length-- > 0)
    {
        output[length++] = value;
    }

    if(length > 0)  writer.advance_write(length);

    return state() == _state_t::OptionValueDone;
}


namespace experimental {

uint8_t ExperimentalPrototypeOptionEncoder1::_option(uint8_t* output_data, option_number_t number, uint16_t length)
{
    moducom::coap::experimental::layer2::OptionBase optionBase(number);

    optionBase.length = length;

    generator.next(optionBase);

    uint8_t pos = 0;

    // OptionValueDone is for non-value version benefit (always reached, whether value is present or not)
    // OptionValue is for value version benefit, as we manually handle value output
    Option::State isDone = length > 0 ? Option::OptionValue : Option::OptionValueDone;

    while (generator.state() != isDone)
    {
        generator_t::output_t output = generator.generate_iterate();

        if (output != generator_t::signal_continue)
            output_data[pos++] = output;
    }

    return pos;
}


void ExperimentalPrototypeBlockingOptionEncoder1::option(pipeline::IPipelineWriter& writer, option_number_t number, uint16_t length)
{
    uint8_t buffer[8];
    // NOTE: Needs underscore on _option as overloading logic can't resolve uint8* vs IPipelineWriter& for some reason
    uint8_t pos = _option(buffer, number, length);

    writer.write(buffer, pos); // generated header-ish portions of option, sans value
}


void
ExperimentalPrototypeBlockingOptionEncoder1::option(pipeline::IPipelineWriter& writer, option_number_t number, const pipeline::MemoryChunk& value)
{
    option(writer, number, value.length());
    writer.write(value); // value portion
}


bool ExperimentalPrototypeNonBlockingOptionEncoder1::option(pipeline::IBufferedPipelineWriter& writer, option_number_t number,
                                                            const pipeline::MemoryChunk& value)
{
    // Need mini-state machine to determine if we are writing value chunk or
    // pre-value chunk
    if (generator.state() == Option::FirstByte)
    {
        _option(buffer, number, value.length());
    }

    // TODO: examine writable to acquire non-blocking count.  Not to be confused with the very similar buffered stuff
    // writable() doesn't exist yet, but should be a part of the base IPipelineWriter - which is looking more and more
    // like a stream all the time
    // NOTE: Probably should draw a distinction between Reader/Writer and PipelineReader/PipelineWriter.  remember,
    // original purpose of pipeline writes were to carry the memory descriptors themselves around.  It seems I've strayed
    // from that due to all the pieces floating around here.  Maybe we need it fully broken out:
    // Reader, BufferedReader, PipelineReader, BufferedPipelineReader
    //writer.writable();
	return true;
}

void ExperimentalPrototypeBlockingPayloadEncoder1::payload(pipeline::IPipelineWriter& writer, const pipeline::MemoryChunk& chunk)
{
    if (!marker_written)
    {
        writer.write(0xFF);
        marker_written = true;
    }

    writer.write(chunk);
}

}

}}
