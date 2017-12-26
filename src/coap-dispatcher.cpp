//
// Created by Malachi Burke on 12/26/17.
//

#include "coap-dispatcher.h"

namespace moducom { namespace coap {

namespace experimental {

void Dispatcher::dispatch(const pipeline::MemoryChunk& chunk)
{
    size_t pos = 0;
    bool process_done = false;

    switch(state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            break;

        case Header:
            // NOTE: Untested code
            while(pos < chunk.length && !process_done)
            {
                process_done = headerDecoder.process_iterate(chunk[pos++]);
            }

            break;

        case HeaderDone:
            dispatch_header();
            if(headerDecoder.token_length() > 0)
                state(Token);
            else
                state(OptionsStart);
            break;

        case Token:
            state(TokenDone);
            break;

        case TokenDone:
            state(OptionsStart);
            break;

        case OptionsStart:
            optionHolder.number_delta = 0;
            optionHolder.length = 0;
            state(Options);
            break;

        case Options:
            // FIX: Not ready for primetime, lots of state not heeded here
            //optionDecoder.process_iterate(chunk, &optionHolder);
            state(OptionsDone);
            break;

        case OptionsDone:
            // TODO: Need to peer into incoming data stream to determine if a payload is on the way
            // or not
            state(Payload);
            break;

        case Payload:
            state(PayloadDone);
            break;

        case PayloadDone:
            state(Done);
            break;

        case Done:
            state(Uninitialized);
            break;
    }
}

void Dispatcher::dispatch_header()
{
    // once we have tagged an interested handler, we should never
    // receive another header
    if(interested)  return;

    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        handler->on_header(headerDecoder);

        if(handler->interested())
        {
            interested = handler;
            return;
        }
    }
}


void Dispatcher::dispatch_option(const pipeline::MemoryChunk& optionChunk)
{
    if(interested)
    {
        //interested->on_option(optionDecoder.option_delta(), optionDecoder.option_length());
        //interested->on_option();
    }
}

}

}}