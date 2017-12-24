//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_PIPELINE_READER_H
#define MC_COAP_TEST_PIPELINE_READER_H

namespace moducom { namespace pipeline {

class IPipelineReader
{
public:
    // returns a memory chunk with NULLS if no pipeline data was present
    virtual PipelineMessage read() = 0;
};


class IBufferedPipelineReader : public IPipelineReader
{
public:
    // inspect what we would get if we performed a read() operation
    // be careful because status operations are still "live" as with
    // a regular read
    virtual PipelineMessage peek() = 0;

    // move read pointer forward, expected consumer will heed
    // result of peek and then advance past "paid attention to"
    // data from peek.  If length == MemoryChunk::length on
    // first iteration of a peek()/advance_read() combo then
    // operation is implicitly identical to regular read()
    virtual bool advance_read(size_t length) = 0;
};


}}

#endif //MC_COAP_TEST_PIPELINE_READER_H
