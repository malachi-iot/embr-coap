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
        uint8_t reserved: 6;
    public:
        bool delivered() const { return _delivered; }
        void delivered(bool value) { _delivered = value; }

        bool copied() const {return _copied;}
        void copied(bool value) { _copied = value; }

        // 8 bits of things just for you!
        uint8_t user : 8;

        // Consider an extended version of this with a semaphore
    };

    Status* status;

    PipelineMessage() : status(NULLPTR) {}
    PipelineMessage(uint8_t* data, size_t length) : status(NULLPTR),
                                                    MemoryChunk(data, length) {}

};


// borrowing concept for .NET low-level pipelining underneath ASP.NET Core
// very similar to message queuing
class IPipeline
{
public:
    // returns a memory chunk with NULLS if no pipeline data was present
    virtual PipelineMessage read() = 0;
    virtual bool write(const PipelineMessage& chunk) = 0;

    bool write(const uint8_t* data, size_t length)
    {
        write(PipelineMessage((uint8_t *)data, length));
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
        if(this->chunk.length) return false;

        this->chunk = chunk;

        return true;
    }
};

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
    IPipeline& incoming;
    IPipeline& outgoing;

public:
    PipelineDaemon(IPipeline& incoming, IPipeline& outgoing)
            :   incoming(incoming),
                outgoing(outgoing) {}

    // process a chunk and return.  May block but tries to avoid it
    // NOTE: all chunks at this time must be FULL packets
    void process_iterate();
};

}}
