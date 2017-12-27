//
// Created by Malachi Burke on 12/26/17.
//

#include "coap-dispatcher.h"

namespace moducom { namespace coap {

namespace experimental {

// TODO: Need a smooth way to ascertain end of CoAP message (remember multipart/streaming
// won't have a discrete message boundary)
// NOTE: This should be dispatch_process since it may well require multiple calls per chunk
//   and consider also returning a bool flag to assist callers in that
bool Dispatcher::dispatch_iterate(Context& context)
{
    const pipeline::MemoryChunk& chunk = context.chunk;
    size_t& pos = context.pos; // how far into chunk our locus of processing should be
    bool process_done = false;

    // FIX: Need to wrap this in a while loop and bump pos up more often

    switch(state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            state(Header);
            new (&headerDecoder) HeaderDecoder;
            break;

        case Header:
            // NOTE: Untested code
            while(pos < chunk.length && !process_done)
            {
                process_done = headerDecoder.process_iterate(chunk[pos]);

                // FIX: This meaning I believe is reversed from non decoder process calls
                //if(!process_done) pos++;

                // FIX: process_done does not designate whether bytes were consumed, for
                // headerDecoder and tokenDecoder, byte is always consumed
                pos++;
            }

            if(process_done) state(HeaderDone);

            break;

        case HeaderDone:
            dispatch_header();
            if(headerDecoder.token_length() > 0)
            {
                state(Token);
                // TODO: May want a TokenStart state
                // NOTE: Initial reset redundant since class initializes with 0, though this
                // may well change when we union-ize the decoders.  Likely though instead we'll
                // use an in-place new
                //new (&tokenDecoder) TokenDecoder();
                tokenDecoder.reset();
            }
            else
                state(OptionsStart);
            break;

        case Token:
            while(pos < chunk.length && !process_done)
            {
                process_done = tokenDecoder.process_iterate(chunk[pos], headerDecoder.token_length());

                // FIX: This meaning I believe is reversed from non decoder process calls
                //if(!process_done) pos++;

                // FIX: process_done does not designate whether bytes were consumed, for
                // headerDecoder and tokenDecoder, byte is always consumed
                pos++;
            }

            if(process_done) state(TokenDone);

            break;

        case TokenDone:
            dispatch_token();

            state(OptionsStart);
            break;

        case OptionsStart:
            // NOTE: reset might be more useful if we plan on not auto-resetting
            // option decoder from within its own state machine
            new (&optionDecoder) OptionDecoder;
            //optionDecoder.reset();
            optionHolder.number_delta = 0;
            optionHolder.length = 0;
            state(Options);
            break;

        case Options:
        {
            pipeline::MemoryChunk optionChunk(chunk.data + pos, chunk.length - pos);
            pos += dispatch_option(optionChunk);

            // FIX: We need to check for the 0xFF payload marker still

            if (optionDecoder.state() == OptionDecoder::OptionValueDone)
            {
                state(OptionsDone);
            }

            break;
        }

        case OptionsDone:
            // TODO: Need to peer into incoming data stream to determine if a payload is on the way
            // or not
            state(Payload);
            break;

        case Payload:
        {
            // FIX: Need this clued in by caller due to streaming possibility
            bool last_payload_chunk = true;
            if(pos == 0)
                dispatch_payload(chunk, last_payload_chunk);
            else
            {
                pipeline::MemoryChunk partialChunk(chunk.data + pos, chunk.length - pos);
                dispatch_payload(partialChunk, last_payload_chunk);
            }
            // pos not advanced since we expect caller to separate out streaming coap payload end from
            // next coap begin and create an artificial chunk boundary
            pos = chunk.length;
            state(PayloadDone);
            break;
        }

        case PayloadDone:
            //dispatch_payload(chunk, true);
            state(Done);
            break;

        case Done:
            state(Uninitialized);
            break;
    }

    // TODO: Do an assert to make sure pos never is >
    ASSERT_ERROR(true, pos <= chunk.length, "pos should never exceed chunk length");

    return pos == chunk.length;
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
size_t Dispatcher::dispatch_option(const pipeline::MemoryChunk& optionChunk)
{
    size_t processed_length = optionDecoder.process_iterate(optionChunk, &optionHolder);

    // FIX: Kludgey, can really be cleaned up and optimized
    // ensures that our linked list doesn't execute process_iterate multiple times to acquire
    // option-value.  Instead, our linked list looping might need to happen per state() switched on
    // Will also cause problems if there are no registered handlers, since count will be off as
    // value is never processed
    bool needs_value_processed = true;
    size_t total_length = processed_length;

    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        if(handler->is_interested())
        {
            switch (optionDecoder.state())
            {
                case OptionDecoder::OptionLengthDone:
                case OptionDecoder::OptionDeltaAndLengthDone:
                {
                    handler->on_option((IOptionInput::number_t) optionHolder.number_delta, optionHolder.length);
                    // TODO: Need to demarkate boundary here too so that on_option knows where boundary starts
                    pipeline::MemoryChunk partialChunk(optionChunk.data + processed_length, 0);

                    if(needs_value_processed)
                    {
                        processed_length = optionDecoder.process_iterate(optionChunk, &optionHolder);
                        total_length += processed_length;
                        needs_value_processed = false;
                    }

                    partialChunk.length = processed_length;
                    bool full_option_value = optionDecoder.state() == OptionDecoder::OptionValueDone;
                    handler->on_option(partialChunk, full_option_value);
                    break;
                }

                case OptionDecoder::OptionValue:
                {
                    // FIX: Not yet tested
                    //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
                    //pipeline::MemoryChunk partialChunk(optionChunk.data, processed_length);
                    //bool full_option_value = optionDecoder.state() == OptionDecoder::OptionValueDone;
                    //handler->on_option(partialChunk, full_option_value);
                    handler->on_option(optionChunk, false);
                    break;
                }

                case OptionDecoder::OptionValueDone:
                {
                    // FIX: Not yet supported
                    // FIX: Not quite correct code.  We need to suss out how large
                    // the chunk we're passing through really is and of course
                    // at what boundary it starts
                    //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
                    handler->on_option(optionChunk, true);
                    break;
                }

                default:break;
            }
        }

        handler = handler->next();
    }

    return total_length;
}


void Dispatcher::dispatch_payload(const pipeline::MemoryChunk& payloadChunk, bool last_chunk)
{
    //size_t processed_length = optionDecoder.process_iterate(payloadChunk, NULLPTR);

    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        if(handler->is_interested())
        {
            handler->on_payload(payloadChunk, last_chunk);
        }
    }
}


void Dispatcher::dispatch_token()
{
    handler_t* handler = head();
    pipeline::MemoryChunk chunk(tokenDecoder.data(), headerDecoder.token_length());

    while(handler != NULLPTR)
    {
        if(handler->is_interested())
        {
            handler->on_token(chunk, true);
        }
    }
}

}

}}
