#pragma once

#include "mc/pipeline.h"
#include "coap_transmission.h"
#include "coap-generator.h"
#include "platform.h"
#include <stdint.h>

namespace moducom { namespace coap {

typedef pipeline::IPipeline IPipeline;

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
    experimental::DispatchingResponder responder;
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
    experimental::DispatchingResponder responder;
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

// Needs a better name.  Represents CoAP messages pushed instead of responded to
// such as (sort of) subscription messages
class Pusher
{
    friend class PipelineDaemon;

    Pusher* next;

public:
    Pusher() : next(NULLPTR) {}

    virtual bool push(IPipeline& output);
};


class PipelineDaemonBase
{
protected:
    // incoming represents data coming into daemon from outside source, outside source could
    // possibly ultimately be lwip or similar
    IPipeline& incoming;
    // outgoing represents data emerging from daemon to outside source,
    IPipeline& outgoing;

    PipelineDaemonBase(IPipeline& incoming, IPipeline& outgoing)
        :       incoming(incoming),
                outgoing(outgoing)
    {}
};

class PipelineDaemon : public PipelineDaemonBase
{
    experimental::DispatchingResponder responder;
    CoAP::ParseToIResponder incoming_parser;
    //CoAPGenerator generator;

    // NOTE: may want to track pushers out in daemon instead of responder
    Pusher* pusher_head;
    Pusher* pusher_tail;

    // services push items only
    bool process_pusher();

    bool has_items_to_push() const { return pusher_head; }

public:
    PipelineDaemon(IPipeline& incoming, IPipeline& outgoing)
            :   PipelineDaemonBase(incoming, outgoing),
                pusher_head(NULLPTR),
                pusher_tail(NULLPTR),
                incoming_parser(&responder)

                //generator(outgoing)
    {}

    // process a chunk and return.  May block but tries to avoid it
    // NOTE: all chunks at this time must be FULL packets
    bool process_iterate();

    void add(Pusher* pusher)
    {
        if(pusher_head == NULLPTR)
        {
            pusher_head = pusher;
            pusher_tail = pusher;
            return;
        }

        pusher_tail->next = pusher;
    }
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

class INonBlockingSender;

namespace experimental {

class NonBlockingPipelineDaemon : public PipelineDaemonBase
{
    CoAP::ParseIterateToIResponder incoming_parser;
    INonBlockingSender* sender;

    enum SendState
    {
        Start,
        SendingOptions,
        SendingPayload,
        Finish
    };

    SendState send_state;

protected:
    // Acquire next sender, or null if none is ready to go
    virtual INonBlockingSender* next_sender() = 0;
    // Acquire next receiver, or null if none is ready to go
    // [extra experimental, pretty sure we need to evaluate incoming header to know which receiver we're itnerested in]
    virtual CoAP::IResponder* next_receiver() = 0;

public:
    NonBlockingPipelineDaemon(IPipeline& incoming, IPipeline& outgoing) :
            PipelineDaemonBase(incoming, outgoing),
            send_state(Start),
            incoming_parser(NULLPTR),
            sender(NULLPTR)
    {}

    // process as much as we can from incoming pipeline without blocking
    // if pipeline is totally empty, return true.
    bool process_incoming();

    // push as much as we can into outgoing pipeline without blocking.
    // if no further pending output, return true
    bool process_outgoing();
};

}


}}
