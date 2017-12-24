//
// Created by Malachi Burke on 12/24/17.
//

#ifndef MC_COAP_TEST_PIPELINE_WRITER_H
#define MC_COAP_TEST_PIPELINE_WRITER_H

#include "memory-chunk.h"

namespace moducom { namespace pipeline {

class IPipelineWriter
{
public:
    // returns true when memory could be written to pipeline,
    // false otherwise
    virtual bool write(const PipelineMessage& chunk) = 0;

    bool write(const uint8_t* data, size_t length)
    {
        return write(PipelineMessage((uint8_t *)data, length));
    }

    bool write(uint8_t ch)
    {
        return write(&ch, 1);
    }

    // TODO: move advanceable (or perhaps rename to canwrite) here
    // to increase non-blocking abilities
};


class IBufferedPipelineWriter : public IPipelineWriter
{
public:
    // Inspect destination buffer where a future write would go
    virtual MemoryChunk peek_write() = 0;

    // Advance write buffer by length amount (expects that
    // peek_write buffer was utilized that much)
    virtual bool advance_write(size_t length) = 0;
};


namespace layer3 {

// TODO: Zero bounds checking, will need that fixed
class SimpleBufferedPipelineWriter : public IBufferedPipelineWriter
{
    pipeline::MemoryChunk buffer;

    // How much memory of the specified buffer is actually available/written to
    size_t length_used;

public:
    SimpleBufferedPipelineWriter(const pipeline::MemoryChunk& buffer)
            :
            buffer(buffer),
            length_used(0)
    {

    }


    virtual bool write(const PipelineMessage& chunk)
    {
        memcpy(buffer.data, chunk.data, chunk.length);
        return true;
    }


    virtual pipeline::MemoryChunk peek_write()
    {
        pipeline::MemoryChunk chunk(buffer.data + length_used, buffer.length - length_used);
        return chunk;
    }


    virtual bool advance_write(size_t length) OVERRIDE
    {
        length_used += length;
        return true;
    }
};


}

}}

#endif //MC_COAP_TEST_PIPELINE_WRITER_H
