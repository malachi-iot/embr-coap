//
// Created by Malachi Burke on 12/25/17.
//

#ifndef MC_COAP_TEST_PIPELINE_DECODER_H
#define MC_COAP_TEST_PIPELINE_DECODER_H

#include "mc/memory-chunk.h"
#include "pipeline-reader.h"

namespace moducom { namespace pipeline { namespace experimental {

//! Connects an input pipline to a decoder (which consumes raw encoded bytes)
//! \tparam TDecoder
template <class TDecoder>
class PipelineDecoderAdapter
{
    IPipelineReader& input;
    TDecoder& decoder;

public:
    bool process_iterate();
};



template <class TDecoder>
bool PipelineDecoderAdapter<TDecoder>::process_iterate()
{
    // TODO: check input.available() once that exists for proper non blocking behavior
    PipelineMessage chunk = input.read();

    if(chunk.length() > 0)
    {
        // TODO: need to hold on to this PipelineMessage somewhere so that we can
        // properly iterate over it.  May also need to establish paradigm where
        // called process_iterate returns how many bytes were actually processed
        // (to advance forward through the message chunk)
        decoder.process_iterate(chunk);
    }
}

//! Connects an input pipline to a decoder (which consumes raw encoded bytes)
//! \tparam TDecoder
template <class TDecoder>
class BufferedPipelineDecoderAdapter
{
    IBufferedPipelineReader& input;
    TDecoder& decoder;

public:
    bool process_iterate();
};


template <class TDecoder>
bool BufferedPipelineDecoderAdapter<TDecoder>::process_iterate()
{
    // TODO: check input.available() once that exists for proper non blocking behavior
    PipelineMessage chunk = input.peek();

    if(chunk.length() > 0)
    {
        // TODO: need to hold on to this PipelineMessage somewhere so that we can
        // properly iterate over it.  May also need to establish paradigm where
        // called process_iterate returns how many bytes were actually processed
        // (to advance forward through the message chunk)
        decoder.process_iterate(chunk);

        // TODO: Need to advance it now too
    }
}



}}}

#endif //MC_COAP_TEST_PIPELINE_DECODER_H
