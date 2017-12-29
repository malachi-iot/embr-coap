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

}}