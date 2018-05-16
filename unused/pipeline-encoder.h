//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_PIPELINE_ENCODER_H
#define MC_COAP_TEST_PIPELINE_ENCODER_H

#include "pipeline.h"

namespace moducom { namespace pipeline {

namespace experimental {

//! Connects an encoder (which spits out bytes) to an output pipeline
//! \tparam TEncoder
template<class TEncoder>
class PipelineEncoderAdapter
{
    IPipelineWriter& output;
    TEncoder& encoder;

    typedef TEncoder encoder_t;
    typedef typename encoder_t::output_t encoder_output_t;

public:
    PipelineEncoderAdapter(IPipelineWriter& output, TEncoder& encoder)
            :
            output(output),
            encoder(encoder) {}


    // returns true when something was processed,
    // false when we cannot process any more at this time
    // FIX: resolve how process_iterate return flag is not homogenous in its
    // behavior (consider using an enum instead)
    bool process_iterate();

    void process() { while (process_iterate()); }
};


template<class TEncoder>
bool PipelineEncoderAdapter<TEncoder>::process_iterate()
{
    encoder_output_t out = encoder.process_iterate();

    if (out == TEncoder::signal_continue) return false;

    // TODO: look into advanceable/buffer code
    output.write(out);

    return true;
}



//! Connects an encoder (which spits out bytes) to an output pipeline
//! \tparam TEncoder
template<class TEncoder>
class BufferedPipelineEncoderAdapter
{
    IBufferedPipelineWriter& output;
    TEncoder& encoder;

    typedef TEncoder encoder_t;
    typedef typename encoder_t::output_t encoder_output_t;

public:
    BufferedPipelineEncoderAdapter(IBufferedPipelineWriter& output, TEncoder& encoder)
            :
            output(output),
            encoder(encoder) {}


    // returns true when something was processed,
    // false when we cannot process any more at this time
    // FIX: resolve how process_iterate return flag is not homogenous in its
    // behavior (consider using an enum instead)
    bool process_iterate();

    void process() { while (process_iterate()); }
};


template<class TEncoder>
bool BufferedPipelineEncoderAdapter<TEncoder>::process_iterate()
{
    encoder_output_t out = encoder.process_iterate();

    if (out == TEncoder::signal_continue) return false;

    MemoryChunk chunk = output.peek_write();

    chunk[0] = out;

    return output.advance_write(1);
}

}

}}

#endif //MC_COAP_TEST_PIPELINE_ENCODER_H
