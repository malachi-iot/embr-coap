//
// Created by Malachi Burke on 11/11/17.
//

#ifndef MC_COAP_TEST_PIPELINE_H
#define MC_COAP_TEST_PIPELINE_H

//#include "../coap_transmission.h"
#include "../platform.h"
#include <stdint.h>
#include <memory.h>


namespace moducom { namespace pipeline {

class MemoryChunkBase
{
public:
    size_t length;
};

class MemoryChunk : public MemoryChunkBase
{
public:
    uint8_t* data;

    MemoryChunk(uint8_t* data, size_t length) : data(data)
    {
        this->length = length;
    }
    MemoryChunk() {}

    inline void memset(uint8_t c, size_t length) { ::memset(data, c, length); }
    inline void memset(uint8_t c) { memset(c, length); }

    inline uint8_t& operator[](size_t index)
    {
        return data[index];
    }
};

// TODO: Oops, this should be layer3 since it has pointer *and* size fields
namespace layer2
{
// TODO: Update code to use layer3, then change this to not extend pipeline::MemoryChunk
template <size_t buffer_length>
class MemoryChunk : public pipeline::MemoryChunk
{
    uint8_t buffer[buffer_length];

public:
    MemoryChunk() : pipeline::MemoryChunk(buffer, buffer_length) {}

    inline void memset(uint8_t c) { ::memset(buffer, c, buffer_length); }
};

}

namespace layer3
{
template <size_t buffer_length>
class MemoryChunk : public pipeline::MemoryChunk
{
    uint8_t buffer[buffer_length];

public:
    MemoryChunk() : pipeline::MemoryChunk(buffer, buffer_length) {}

    inline void memset(uint8_t c) { ::memset(buffer, c, buffer_length); }
};

}


class PipelineMessage : public MemoryChunk
{
    // One might think of Status vs CopiedStatus as WriteStatus vs ReadStatus
public:
    // Expected this will live on a stack or instance field somewhere
    // With this we can retrieve how our message is progressing through
    // the pipelines
    struct Status
    {
    private:
        // message has reached its final destination
        bool _delivered : 1;
        // message has been copied and original no longer
        // needed
        bool _copied : 1;

        // 6 bits of things just for me!
        // TODO: Consider a flag for memory handle
        // TODO: Consider also a cheap-n-nasty RTTI for extended Statuses
        // TODO: Consider a demarcation flag to denote a packet boundary.  This gets a little tricky since
        //       some scenarios have many different boundaries to consider
        uint8_t reserved: 6;
    public:
        bool delivered() const { return _delivered; }
        void delivered(bool value) { _delivered = value; }

        bool copied() const {return _copied;}
        void copied(bool value) { _copied = value; }

        // different from regular copy in that only the message itself has been copied
        // used primarily for by-reference messages vs by-value messages.  Not sure if
        // a) we're gonna even use by-ref messages and b) if we do whether this will be
        // useful
        bool experimental_shallow_copied() const { return false; }

        // 8 bits of things just for you!
        uint8_t user : 8;

        // Consider an extended version of this with a semaphore
    };

    // this is like regular Status except it's merely copied along with each message,
    // thus partially alleviating memory management issue with regular Status*
    // naturally, because of this, copied_status can't be used to report status back to
    // the originator
    struct CopiedStatus
    {
        // // 8 bits of things just for you!
        // note in buffered situations, only the last user status is carried.  This contradicts
        // boundary behavior described below, so plan accordingly when using boundaries
        uint32_t user : 8;

        // boundary indicates a signal to a pipeline consumer (the one doing reads) that
        // possibly an action needs to be taken (boundary reached)
        // boundaries tend to be application specific, so use this for boundary types 1-7, 0 being
        // no boundary.  Only restriction is that in buffered situations, larger boundary types will "eat"
        // smaller ones until buffer is read back out again:
        //
        // ...1...1...2...3...1...2...1
        //                ^       ^
        // buffer read ---+     --+
        //
        // becomes
        // ...............3.......2...
        uint32_t boundary : 3;

        // Helper flag to indicate whether get_buffer was utilized, in which case
        // we should take extra measures to avoid buffer allocation and/or copying
        uint32_t is_preferred_experimental : 1;
    } copied_status;

    Status* status;

    PipelineMessage() : status(NULLPTR) {}
    PipelineMessage(uint8_t* data, size_t length) : status(NULLPTR),
                                                    MemoryChunk(data, length)
    {
        copied_status.user = 0;
        copied_status.boundary = 0;
    }

    PipelineMessage(const MemoryChunk& chunk) : MemoryChunk(chunk), status(NULLPTR) {}

};


class IPipelineReader
{
public:
    // returns a memory chunk with NULLS if no pipeline data was present
    virtual PipelineMessage read() = 0;
};


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
        chunk.data = 0;
        return return_chunk;
    }

    virtual bool write(const PipelineMessage& chunk) OVERRIDE
    {
        // can't write to chunk because spot is filled
        if(this->chunk.length) return false;

        this->chunk = chunk;

        return true;
    }
};


namespace experimental {


//! Connects an encoder (which spits out bytes) to an output pipeline
//! \tparam TEncoder
template <class TEncoder>
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
    encoder(encoder)
    {}


    bool process_iterate();

    void process() { while(!process_iterate()); }
};


template <class TEncoder>
bool PipelineEncoderAdapter<TEncoder>::process_iterate()
{
    encoder_output_t out = encoder.process_iterate();

    if(out == TEncoder::signal_continue) return false;

    // TODO: look into advanceable/buffer code
    output.write(out);

    return true;
}


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

    if(chunk.length > 0)
    {
        // TODO: need to hold on to this PipelineMessage somewhere so that we can
        // properly iterate over it.  May also need to establish paradigm where
        // called process_iterate returns how many bytes were actually processed
        // (to advance forward through the message chunk)
        decoder.process_iterate(chunk);
    }
}


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
class SimpleBufferedPipeline : public IPipeline
{
    pipeline::MemoryChunk buffer;
    size_t length_used;

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
    SimpleBufferedPipeline(const pipeline::MemoryChunk& buffer) :
            buffer(buffer),
            length_used(0)
    {
        copied_status.boundary = 0;
    }

    virtual PipelineMessage read() OVERRIDE
    {
        PipelineMessage msg;

        if(length_used == 0)
            msg.data = NULLPTR;
        else
            msg.data = buffer.data;

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

            memcpy(buffer.data + length_used, chunk.data, chunk.length);

            length_used += chunk.length;

            if(chunk.status)
                chunk.status->copied(true);

            return true;
        }

        return false;
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
            msg.data = NULLPTR;
        else
            msg.data = buffer.data;

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
        size_t delta = chunk.data - buffer.data;

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

    // FIX
    virtual size_t advanceable() OVERRIDE { return 99999; }

};

}

}

}}

#endif //MC_COAP_TEST_PIPELINE_H
