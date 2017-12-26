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
            dispatch_option(chunk);
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
            dispatch_payload(chunk);
            state(Done);
            break;

        case Done:
            state(Uninitialized);
            break;
    }
}

void Dispatcher::dispatch_header()
{
    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        handler->on_header(headerDecoder);
        handler = handler->next();
    }
}

// also handles pre-dispatch processing
// 100% untested
void Dispatcher::dispatch_option(const pipeline::MemoryChunk& optionChunk)
{
    optionDecoder.process_iterate(optionChunk, &optionHolder);

    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        if(handler->is_interested())
        {
            switch (optionDecoder.state())
            {
                case OptionDecoder::OptionLengthDone:
                case OptionDecoder::OptionDeltaAndLengthDone:
                    handler->on_option((IOptionInput::number_t) optionHolder.number_delta, optionHolder.length);
                    // TODO: Need to demarkate boundary here too so that on_option knows where boundary starts
                    break;

                case OptionDecoder::OptionValue:
                    // Not yet supported
                    break;

                case OptionDecoder::OptionValueDone:
                {
                    // FIX: Not quite correct code.  We need to suss out how large
                    // the chunk we're passing through really is and of course
                    // at what boundary it starts
                    pipeline::PipelineMessage msg(optionChunk);
                    handler->on_option(&msg);
                    break;
                }

                default:break;
            }
        }

        handler = handler->next();
    }
}


void Dispatcher::dispatch_payload(const pipeline::MemoryChunk& payloadChunk)
{
    
}

}

}}