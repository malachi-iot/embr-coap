#pragma once

#include "coap_transmission.h"
#include "platform.h"
#include <stdint.h>

namespace moducom { namespace coap {


class MemoryChunk
{
public:
    uint8_t* data;
    size_t length;

    MemoryChunk(uint8_t* data, size_t length) : data(data), length(length) {}
    MemoryChunk() {}
};


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
    } copied_status;

    Status* status;

    PipelineMessage() : status(NULLPTR) {}
    PipelineMessage(uint8_t* data, size_t length) : status(NULLPTR),
                                                    MemoryChunk(data, length)
    {
        copied_status.user = 0;
        copied_status.boundary = 0;
    }

};


// borrowing concept for .NET low-level pipelining underneath ASP.NET Core
// very similar to message queuing
class IPipeline
{
public:
    // returns a memory chunk with NULLS if no pipeline data was present
    virtual PipelineMessage read() = 0;

    // returns true when memory could be written to pipeline,
    // false otherwise
    virtual bool write(const PipelineMessage& chunk) = 0;

    bool write(const uint8_t* data, size_t length)
    {
        return write(PipelineMessage((uint8_t *)data, length));
    }
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
    MemoryChunk buffer;
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
    SimpleBufferedPipeline(const MemoryChunk& buffer) :
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

}

// NOTE: likely would be much better to use the util.embedded streams,
// but they are so arranged to be low-level it would take a lot of adaptation
// for them to work here.  Still, I'd prefer using them
class IStreamReader
{
public:
    // how many bytes are available (up until the next packet boundary)
    virtual size_t available() = 0;

    virtual size_t read(uint8_t* data, size_t length) = 0;
};


class IStreamWriter
{
    //!
    //! \param data
    //! \param length
    //! \param packet_boundary denotes the end of a packet.  If write didn't write everything
    //                         keep sending only the remaining packet portion of length and
    //                         continue to flag this as true
    //! \return
    virtual size_t write(const uint8_t* data, size_t length) = 0;
};

class IStream :
        public IStreamReader,
        public IStreamWriter
{
public:
};


class IStreamPacketInfo
{
public:
    virtual void write_packet(); // signal the end of one packet of writing
    // indicates at least one full packet is available, especially when reads are returning 0
    // auto-advance if true to the next packet if available *and* we are at the end, unless advance = false
    virtual bool read_packet(bool advance = false);
};

class DaemonBase
{
protected:
    experimental::TestResponder responder;
    CoAP::ParseToIResponder incoming_parser;
    experimental::TestOutgoingMessageHandler outgoing_generator;

public:
    DaemonBase() :
            responder(&outgoing_generator),
            incoming_parser(&responder) {}
};

// pretty sure I DONT like all the copying around this implies
class PassiveDaemon :
        public IStream,
        public DaemonBase
{
public:
    // Ultimately one can piecemeal read or write, but currently
    // as that's a complex task right now read/write must operate on an entire packet at once

    // write data that came in from transport into the daemon
    virtual size_t write(const uint8_t* data, size_t length) OVERRIDE;

    // read out data that the daemon has processed
    virtual size_t read(uint8_t* data, size_t length) OVERRIDE;

    virtual size_t available() OVERRIDE { return 0; }
};


class Daemon
{
    experimental::TestResponder responder;
    CoAP::ParseToIResponder incoming_parser;
    experimental::TestOutgoingMessageHandler outgoing_generator;
    IStream& stream;
    IStreamPacketInfo* stream_packet;

public:
    Daemon(IStream& stream, IStreamPacketInfo* stream_packet = NULLPTR) :
            stream_packet(stream_packet),
            incoming_parser(&responder),
            stream(stream) {}

    // process a chunk and return.  May block, but tries to avoid it
    void process_iterative();

};

class PipelineDaemon : public DaemonBase
{
    // incoming represents data coming into daemon from outside source, outside source could
    // possibly ultimately be lwip or similar
    IPipeline& incoming;
    // outgoing represents data emerging from daemon to outside source,
    IPipeline& outgoing;

public:
    PipelineDaemon(IPipeline& incoming, IPipeline& outgoing)
            :   incoming(incoming),
                outgoing(outgoing) {}

    // process a chunk and return.  May block but tries to avoid it
    // NOTE: all chunks at this time must be FULL packets
    void process_iterate();
};

// Daemon where external control loop initiates pushes into "incoming"
class PassivePipelineDaemon : public PipelineDaemon
{
public:
    PassivePipelineDaemon(IPipeline& incoming, IPipeline& outgoing) :
            PipelineDaemon(incoming, outgoing) {}
};



// Daemon itself runs control loop and pulls from "incoming"
class ActivePipelineDaemon : public PipelineDaemon
{
public:
    ActivePipelineDaemon(IPipeline& incoming, IPipeline& outgoing) :
        PipelineDaemon(incoming, outgoing) {}

};


}}
