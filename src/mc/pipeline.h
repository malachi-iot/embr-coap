//
// Created by Malachi Burke on 11/11/17.
//

#ifndef MC_COAP_TEST_PIPELINE_H
#define MC_COAP_TEST_PIPELINE_H

//#include "../coap_transmission.h"
#include "../platform.h"
#include <stdint.h>
#include <string.h>
#include "memory-chunk.h"
#include "pipeline-reader.h"
#include "pipeline-writer.h"

namespace moducom { namespace pipeline {

// borrowing concept for .NET low-level pipelining underneath ASP.NET Core
// very similar to message queuing
class IPipeline :
        public IPipelineReader,
        public IPipelineWriter
{
};

// TODO: Find a better name
// With pipelining + message status, we can write one set of code which can either
//
// 1. send a message and block waiting for delivery (traditional blocking)
// 2. send a message and proceed, checking status for delivery (traditional threaded/async)
// 3. send a message and proceed, checking status for just copied (traditional big-buffer style)
class SemiBlockingPipeline : public IPipeline
{
    IPipeline& underlying;

public:
    SemiBlockingPipeline(IPipeline& underlying) : underlying(underlying) {}

    virtual PipelineMessage read() OVERRIDE
    {
        PipelineMessage message;

        do
        {
            message = underlying.read();
            // put a yield into here also
        } while(message.length == 0);

        return message;
    }

    virtual bool write(const PipelineMessage &chunk) OVERRIDE
    {
        bool result = underlying.write(chunk);

        if(result == false) return false;

        if(chunk.status)
            while(!(chunk.status->copied() || chunk.status->delivered()))
            {
                // do some kind of yield
            }

        return true;
    }

};

// no buffering or queuing (just the one chunk), be careful!
class BasicPipeline : public IPipeline
{
    // just the one chunk
    PipelineMessage chunk;

public:
    BasicPipeline() { chunk.length = 0; }

    virtual PipelineMessage read() OVERRIDE
    {
        PipelineMessage return_chunk = chunk;
        chunk.length = 0;
        chunk.data(0);
        return return_chunk;
    }

    virtual bool write(const PipelineMessage& chunk) OVERRIDE
    {
        // can't write to chunk because spot is filled
        if(this->chunk.length) return false;

        this->chunk = chunk;

        return true;
    }

    virtual bool write_experimental(const pipeline::MemoryChunk& chunk) OVERRIDE
    {
        PipelineMessage message(chunk);
        return write(chunk);
    }
};


namespace experimental {



// Augmented pipeline which also provides a preferred buffer (similar to a
// malloc, but it's implied the buffer has already been allocated that is
// returned by get_buffer)
//
// Using this kind of pipeline carries a special behavior which is that
// write operations are expected to carry a MemoryChunk which is contained
// within the get_buffer provided MemoryChunk.  If MemoryChunk being written
// does *not* reside within provided chunk, a copy into a buffer is anticipated
class IBufferProviderPipeline : public IPipeline
{
public:
    // Acquire a buffer provided by this pipeline that later one may
    // conveniently use during a write operation.  Experimental only
    // NOTE: probably will need a free_buffer also
    virtual PipelineMessage get_buffer(size_t size) = 0;

    // Request whether usage of get_buffer-provided buffer is preferred,
    // or whether caller should provide their own
    virtual bool is_buffer_preferred(size_t size) = 0;

    // access internal memory buffer (experimental replacement for other
    // get_buffer)
    virtual bool get_buffer(MemoryChunk** memory_chunk) = 0;

    // move forward in buffer, provider will return next position
    // (experimental for moving forward without doing a write)
    virtual MemoryChunk advance(size_t size, PipelineMessage::CopiedStatus status) = 0;

    // Amount of non-blocking forward movement we can do
    virtual size_t advanceable() = 0;
};


// PipelineMessage is starting to get big, experiental code here to handle
// copying by reference instead of value

class IReferencedReadPipeline
{
    virtual const PipelineMessage& read() = 0;
};

class QueuedReferencePipeline : public IReferencedReadPipeline
{
protected:
    PipelineMessage queue[10];

public:
    virtual const PipelineMessage& read() OVERRIDE
    {
        return queue[0];
    }

    bool write(const PipelineMessage& message)
    {
        queue[0] = message;
        return true;
    }
};

// Convert from experimental by-ref pipeline to regular pipeline
class ConverterPipeline : public QueuedReferencePipeline
{
    IPipeline& pipeline;

public:
    ConverterPipeline(IPipeline& pipeline) : pipeline(pipeline) {}

    virtual const PipelineMessage& read() OVERRIDE
    {
        return queue[0] = pipeline.read();
    }

    bool write(const PipelineMessage& message)
    {
        return pipeline.write(message);
    }
};

}


namespace layer3
{



// No fancy queue-like behavior, just fills a buffer and expects buffer to be
// emptied completely out before new space is available
// FIX: IBufferedPipelineReader is 100% experimental
class SimpleBufferedPipeline : public IPipeline,
                               public IBufferedPipelineReader
{
    pipeline::MemoryChunk buffer;
    // How much memory of the specified buffer is actually available/written to
    size_t length_used;

    // FIX: kludgey just to get buffered pipeline reader happy, represents how
    // far into buffer we've read so far
    size_t length_read;

    // Be careful though, because holding a status that we send out
    // with the read operation may get overwritten by an incoming
    // write operation, so we need some kind of indicator that the
    // one doing the reading is done with the status
    // Seems sensible to rely on the delivered flag for this, but
    // then we can easily get a poison-message-ish situation where
    // the last consumer does not set the delivered flag
    PipelineMessage::Status status;
    PipelineMessage::CopiedStatus copied_status;

public:
    SimpleBufferedPipeline(const pipeline::MemoryChunk& buffer, size_t length_used = 0) :
            buffer(buffer),
            length_used(length_used),
            length_read(0)
    {
        copied_status.boundary = 0;
    }


    virtual PipelineMessage peek() OVERRIDE
    {
        PipelineMessage msg;

        if(length_used == 0)
            msg.data(NULLPTR);
        else
            msg.data(buffer.__data() + length_read);

        msg.length = length_used - length_read;
        msg.status = NULLPTR;
        msg.copied_status = copied_status;

        return msg;
    }


    virtual bool advance_read(size_t count) OVERRIDE
    {
        length_read += count;
        return true;
    }

    virtual PipelineMessage read() OVERRIDE
    {
        PipelineMessage msg;

        if(length_used == 0)
            msg.data(NULLPTR);
        else
            msg.data(buffer.__data());

        msg.length = length_used;
        msg.status = NULLPTR;
        msg.copied_status = copied_status;

        length_used = 0;

        return msg;
    }

    virtual bool write(const PipelineMessage& chunk) OVERRIDE
    {
        if(chunk.length < (buffer.length - length_used))
        {
            if(chunk.copied_status.boundary > copied_status.boundary)
                copied_status.boundary = chunk.copied_status.boundary;

            copied_status.user = chunk.copied_status.user;

            memcpy(buffer.__data() + length_used, chunk.data(), chunk.length);

            length_used += chunk.length;

            if(chunk.status)
                chunk.status->copied(true);

            return true;
        }

        return false;
    }

    virtual bool write_experimental(const pipeline::MemoryChunk& chunk) OVERRIDE
    {
        PipelineMessage message(chunk);
        return write(chunk);
    }

};

namespace experimental {

// A copy/paste hack job from SimpleBufferPipeline
class BufferProviderPipeline : public pipeline::experimental::IBufferProviderPipeline
{
    PipelineMessage buffer;
    size_t length_used;
    PipelineMessage::CopiedStatus copied_status;

public:
    BufferProviderPipeline(const pipeline::MemoryChunk& chunk) : buffer(chunk)
    {
    }

    virtual PipelineMessage get_buffer(size_t size) OVERRIDE
    {
        // FIX: if size is larger than buffer, don't return it -
        // maybe return a null buffer indicating unable to do it
        return buffer;
    }

    virtual pipeline::MemoryChunk advance(size_t length, PipelineMessage::CopiedStatus copied_status) OVERRIDE;

    virtual bool get_buffer(pipeline::MemoryChunk** chunk) OVERRIDE;

    virtual bool is_buffer_preferred(size_t size) OVERRIDE
    {
        return true;
    }

    virtual PipelineMessage read() OVERRIDE
    {
        PipelineMessage msg;

        if(length_used == 0)
            msg.data(NULLPTR);
        else
            msg.data(buffer.__data());

        msg.length = length_used;
        msg.status = NULLPTR;
        msg.copied_status = copied_status;

        length_used = 0;

        return msg;
    }

    virtual bool write(const PipelineMessage& chunk) OVERRIDE
    {
        // delta represents how far into internal buffer.data incoming chunk
        // represents
        size_t delta = chunk.data() - buffer.data();

        if(delta >= buffer.length)
        {
            // incoming chunk does NOT reside in our internally held buffer
        }
        else // delta < buffer.length, chunk *starts* within internally held buffer
        {
            // remaining size means how much internal buffer is available based on
            // position of incoming chunk
            size_t remaining_length = buffer.length - delta;

            // if remaining length of internal buffer meets or exceeds size of
            // incoming chunk, then we are confident incoming chunk fully resides
            // within internal buffer
            if(remaining_length >= chunk.length)
            {
                // all good
            }
            else
            {
                // error condition, part of buffer is ours, part is theirs
            }
        }

        return true;
    }

    virtual bool write_experimental(const pipeline::MemoryChunk& chunk) OVERRIDE
    {
        PipelineMessage message(chunk);
        return write(chunk);
    }

    // FIX
    virtual size_t advanceable() OVERRIDE { return 99999; }

};

}

}

}}

#endif //MC_COAP_TEST_PIPELINE_H
