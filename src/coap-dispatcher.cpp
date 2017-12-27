//
// Created by Malachi Burke on 12/26/17.
//

#include "coap-dispatcher.h"

namespace moducom { namespace coap {

namespace experimental {

// TODO: Need a smooth way to ascertain end of CoAP message (remember multipart/streaming
// won't have a discrete message boundary)
bool Dispatcher::dispatch_iterate(Context& context)
{
    const pipeline::MemoryChunk& chunk = context.chunk;
    size_t& pos = context.pos; // how far into chunk our locus of processing should be
    bool process_done = false;

    switch(state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            state(Header);
            init_header_decoder();
            break;

        case Header:
            while(pos < chunk.length && !process_done)
            {
                process_done = headerDecoder.process_iterate(chunk[pos]);

                // FIX: This meaning I believe is reversed from non decoder process calls
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
                init_token_decoder();
            }
            else
                state(OptionsStart);
            break;

        case Token:
            while(pos < chunk.length && !process_done)
            {
                // TODO: Utilize a simpler counter and chunk out token
                process_done = tokenDecoder.process_iterate(chunk[pos], headerDecoder.token_length());

                // FIX: This meaning I believe is reversed from non decoder process calls
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
            init_option_decoder();
            optionHolder.number_delta = 0;
            optionHolder.length = 0;
            state(Options);
            break;

        case Options:
        {
            pos += dispatch_option(chunk.remainder(pos));

            // handle option a.1), a.2) or b.1) described below
            if ((pos == chunk.length && context.last_chunk) || chunk[pos] == 0xFF)
            {
                ASSERT_ERROR(OptionDecoder::OptionValueDone, optionDecoder.state(), "Must always be optionValueDone here");
                // will check again for 0xFF
                state(OptionsDone);
            }
            else
            {
                // UNSUPPORTED
                // b.2 means we are not sure if another option or an incoming chunk[0] == 0xFF is coming
                // MAY need an OptionsBoundary state, but hopefully not
                // better would be to beef up OptionDecoder awareness of Payload marker
            }

            break;
        }

        case OptionsDone:
            // TODO: Need to peer into incoming data stream to determine if a payload is on the way
            // or not - specifically need to also check size.  Here there are 3 possibilities:
            //
            // a.1) payload marker present, so process a payload, entire chunk / datagram remainder present
            // a.2) payload marker present, but only partial chunk present, so more payload to process
            // b.1) end of datagram - entire chunk present
            // b.2) end of chunk - only partial chunk present
            // c) as-yet-to-be-determined streaming end of chunk marker
            if (pos == chunk.length && context.last_chunk)
            {
                // this is for condition b.1)
                state(Done);
            }
            else if (chunk[pos] == 0xFF)
            {
                pos++;

                // this is for condition a.1 or 1.2
                state(Payload);
            }
            else
            {
                // UNSUPPORTED
                // falls through to condition c, but could get a false positive on header 0xFF
                // so this is unsupported.  Plus we can't really get here properly from Options state
                state(Done);
            }
            break;

        case Payload:
        {
            bool last_payload_chunk = context.last_chunk;

            if(pos == 0)
                // arrives here in the unlikely case payload starts on new chunk
                // boundary
                dispatch_payload(chunk, last_payload_chunk);
            else
                // typically represents an a.1 scenario, but doesn't have to
                dispatch_payload(chunk.remainder(pos), last_payload_chunk);

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
    size_t value_pos = processed_length;

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

                    if(needs_value_processed)
                    {
                        processed_length = optionDecoder.process_iterate(optionChunk, &optionHolder);
                        total_length += processed_length;
                        needs_value_processed = false;
                    }

                    // TODO: Need to demarkate boundary here too so that on_option knows where boundary starts
                    pipeline::MemoryChunk partialChunk(optionChunk.data + value_pos, processed_length);

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

        handler = handler->next();
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
            // this dispatcher does not chunk out token, we gather the whole thing in a local buffer
            // FIX: really though we SHOULD chunk it out (if necessary) and avoid allocating a buffer
            // entirely - under the right circumstances
            // for character-by-character scenarios though we DO prefer to have this mini-buffered
            // version.  As such, we probably want a separate dispatcher which is optimized for character
            // by character dispatching which uses this TokenDecoder
            handler->on_token(chunk, true);
        }

        handler = handler->next();
    }
}

}

}}
