//
// Created by Malachi Burke on 11/24/17.
//

#include "coap-encoder.h"

namespace moducom { namespace coap {

bool OptionEncoder::process_iterate(pipeline::IBufferedPipelineWriter& writer)
{
    pipeline::PipelineMessage output = writer.peek_write();

    output_t value;
    size_t length = 0;

    while((value = generate_iterate() != signal_continue) && output.length-- > 0)
    {
        output[length++] = value;
    }

    if(length > 0)  writer.advance_write(length);

    return state() == CoAP::Parser::OptionValueDone;
}


namespace experimental {

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


void ExperimentalPrototypeBlockingOptionEncoder1::option(pipeline::IPipelineWriter& writer, number_t number, uint16_t length)
{
    uint8_t buffer[8];
    // NOTE: Needs underscore on _option as overloading logic can't resolve uint8* vs IPipelineWriter& for some reason
    uint8_t pos = _option(buffer, number, length);

    writer.write(buffer, pos); // generated header-ish portions of option, sans value
}


void
ExperimentalPrototypeBlockingOptionEncoder1::option(pipeline::IPipelineWriter& writer, number_t number, const pipeline::MemoryChunk& value)
{
    option(writer, number, value.length);
    writer.write(value); // value portion
}


bool ExperimentalPrototypeNonBlockingOptionEncoder1::option(pipeline::IBufferedPipelineWriter& writer, number_t number,
                                                            const pipeline::MemoryChunk& value)
{
    // Need mini-state machine to determine if we are writing value chunk or
    // pre-value chunk
    if (generator.state() == CoAP::Parser::OptionSize)
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