//
// Created by malachi on 10/29/17.
//

#include "coap_daemon.h"

using namespace moducom::pipeline;

namespace moducom { namespace coap {


size_t PassiveDaemon::write(const uint8_t* data, size_t length)
{
    incoming_parser.process(data, length);
    return length;
}



size_t PassiveDaemon::read(uint8_t* data, size_t length)
{
    return 0;
}


void Daemon::process_iterative()
{
    if(stream_packet)
    {
        if(stream_packet->read_packet())
        {

        }
    }
}


bool PipelineDaemon::process_iterate()
{
    // FIX: need to make this read/incoming_parser.process a process_iterate
    MemoryChunk chunk = incoming.read();

    incoming_parser.process(chunk.data, chunk.length);

    //outgoing.write(outgoing_generator.get_buffer(), outgoing_generator.get_pos());
    // if push code exists
    if(pusher_head)
    {
        // execute it iteratively
        if(!pusher_head->push(outgoing)) return false;

        // if iterative execution of push head is complete, then advance to next
        // push if present
        pusher_head = pusher_head->next;
        if(pusher_head == NULLPTR) pusher_tail = NULLPTR;

        return true;
    }
}

}}