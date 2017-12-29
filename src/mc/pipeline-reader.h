//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_PIPELINE_READER_H
#define MC_COAP_TEST_PIPELINE_READER_H

namespace moducom { namespace pipeline {

namespace experimental {

struct IReader
{
    // regular classic stream oriented read
    virtual size_t read_experimental(MemoryChunk& chunk);
};

}

class IPipelineReader
{
public:
    // returns a memory chunk with NULLS if no pipeline data was present
    // TODO: Consider turning this into a more traditional read(buf, len) operation and save
    // the more advanced one for IBufferedPipelineReader
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
