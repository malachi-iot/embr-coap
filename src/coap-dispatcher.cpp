//
// Created by Malachi Burke on 12/26/17.
//

#include "coap-dispatcher.h"
#include "coap/context.h"

#ifdef ESP_DEBUG
#include <stdio.h>
#endif


namespace moducom { namespace coap {

namespace experimental {

// TODO: Eventually clean up dispatch_option and then
// just run process_iterate always at the bottom
bool Dispatcher::dispatch_iterate(Context& context)
{
    const pipeline::MemoryChunk::readonly_t& chunk = context.chunk;
    size_t& pos = context.pos; // how far into chunk our locus of processing should be

    switch(state())
    {
        case HeaderDone:
            dispatch_header();
            process_iterate(context);
            break;

        case TokenDone:
            dispatch_token();
            process_iterate(context);
            break;

        case Options:
        {
            // Repeating ourselves (and not calling) from decoder because of the special way in which
            // dispatcher reads out options
#ifdef DEBUG2
            std::clog << __func__ << ": 1 optionDecoder state = " << (optionDecoder.state()) << std::endl;
#endif

            pos += dispatch_option(chunk.remainder(pos));

#ifdef DEBUG2
            std::clog << __func__ << ": 2 optionDecoder state = " << (optionDecoder.state()) << std::endl;
#endif

            // handle option a.1), a.2) or b.1) described below
            if ((pos == chunk.length() && context.last_chunk) || chunk[pos] == 0xFF)
            {
                ASSERT_ERROR(true,
                             (optionDecoder.state() == OptionDecoder::OptionValueDone) ||
                             (optionDecoder.state() == OptionDecoder::OptionDeltaAndLengthDone),
                             "Must be either optionValueDone or optionDeltaAndlengthDone.  Got: " << optionDecoder.state());
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

            process_iterate(context);
            break;
        }

        default:
            process_iterate(context);
            break;
    }

    // TODO: Do an assert to make sure pos never is >
    ASSERT_ERROR(true, pos <= chunk.length(), "pos should never exceed chunk length");

    return pos == chunk.length();
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
size_t Dispatcher::dispatch_option(const pipeline::MemoryChunk::readonly_t& optionChunk)
{
    // FIX: we generally expect to be at FirstByte state here, however in the future this
    // requirement should go away (once we clean up needs_value_processed behavior)
    size_t processed_length = optionDecoder.process_iterate(optionChunk, &optionHolder);
    size_t value_pos = processed_length;
    OptionDecoder& option_decoder = this->option_decoder();

    // FIX: Kludgey, can really be cleaned up and optimized
    // ensures that our linked list doesn't execute process_iterate multiple times to acquire
    // option-value.  Instead, our linked list looping might need to happen per state() switched on
    // Will also cause problems if there are no registered handlers, since count will be off as
    // value is never processed
    bool needs_value_processed = true;
    size_t total_length = processed_length;
#ifdef FEATURE_DISCRETE_OBSERVERS
    typedef IOptionObserver::number_t option_number_t;
#else
    typedef IMessageObserver::number_t option_number_t;
#endif

    handler_t* handler = head();

    while(handler != NULLPTR)
    {
        if(handler->is_interested())
        {
            switch (option_decoder.state())
            {
                //case OptionDecoder::OptionLengthDone:
                //case OptionDecoder::OptionDeltaAndLengthDone:
                case OptionDecoder::ValueStart:
                {
                    option_number_t option_number = (option_number_t) optionHolder.number_delta;
                    uint16_t option_length = optionHolder.length;
#ifdef DEBUG2
                    std::clog << "Dispatching option: " << option_number << " with length " << optionHolder.length;
                    std::clog << std::endl;
#endif
                    handler->on_option(option_number, option_length);

                    if(needs_value_processed)
                    {
                        // Getting here
                        processed_length = option_decoder.process_iterate(optionChunk.remainder(processed_length), &optionHolder);
                        total_length += processed_length;
                        needs_value_processed = false;
                    }

                    // this will happen normally during multi-chunk operations
                    // we shouldn't be seeing that for a while, however
                    ASSERT_WARN(option_length, processed_length, "option length mismatch with processed bytes");

                    if(option_length > 0)
                    {
                        // TODO: Need to demarkate boundary here too so that on_option knows where boundary starts
                        pipeline::MemoryChunk partialChunk((uint8_t*)(optionChunk.data()) + value_pos, processed_length);

#ifdef DEBUG2
                        std::clog << "Dispatching option: data = ";

                        // once we resolve the two different OptionExperimentals, use this
                        //CoAP::OptionExperimentalDeprecated::get_format()
                        //optionHolder.get_format();
                        // for now, just assume everything is a string
                        for(int i = 0; i < option_length; i++)
                        {
                            std::clog << (char)partialChunk[i];
                        }

                        std::clog << std::endl;
#endif
                        // FIX: We are arriving here with values of FirstByte(Done?) and OptionValue
                        // suggesting strongly we aren't iterating completely or fast-forwarding past
                        // value when we need to
                        bool full_option_value = option_decoder.state() == OptionDecoder::OptionValueDone;
                        handler->on_option(option_number, partialChunk, full_option_value);
                    }
                    break;
                }

                case OptionDecoder::FirstByteDone:

                    break;

                case OptionDecoder::OptionValue:
                {
                    // FIX: Not yet tested
                    //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
                    //pipeline::MemoryChunk partialChunk(optionChunk.data, processed_length);
                    //bool full_option_value = optionDecoder.state() == OptionDecoder::OptionValueDone;
                    //handler->on_option(partialChunk, full_option_value);
                    option_number_t option_number = (option_number_t) optionHolder.number_delta;
                    handler->on_option(option_number, optionChunk, false);
                    break;
                }

                case OptionDecoder::OptionValueDone:
                {
                    // FIX: Not yet supported
                    // FIX: Not quite correct code.  We need to suss out how large
                    // the chunk we're passing through really is and of course
                    // at what boundary it starts
                    //processed_length = optionDecoder.process_iterate(optionChunk, NULLPTR);
                    option_number_t option_number = (option_number_t) optionHolder.number_delta;
                    handler->on_option(option_number, optionChunk, true);
                    break;
                }

                case OptionDecoder::Payload:
                    // NOTE: Not yet used, we don't iterate enough to get here
                    break;

                default:break;
            }
        }

        handler = handler->next();
    }

    return total_length;
}


void Dispatcher::dispatch_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk)
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



void FactoryDispatcherHandler::on_header(Header header)
{
#ifdef ESP_DEBUG
    printf("\r\nFactoryDispatcherHandler::on_header type=%d, mid=%X",
        header.type(), header.message_id());
#endif

    // NOTE: Very unlikely that we'd dispatch on header itself, but conceivable
    for(int i = 0; i < handler_factory_count; i++)
    {
        context_t ctx(context.incoming_context, handler_memory());

        IDispatcherHandler* handler = handler_factories[i](handler_memory());
        State& state = handler_state(i);

        if(!state.is_interested() && state.initialized()) continue;

        handler->on_header(header);

        state.interested(handler->interested());

        if(handler->is_always_interested())
        {
            chosen = handler;
            return; // obviously do NOT destruct the chosen handler
        }

        // placement new demands explicit destructor invocation
        handler->~IDispatcherHandler();
    }
}



void FactoryDispatcherHandler::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{
#ifdef ESP_DEBUG
    printf("\r\nFactoryDispatcherHandler::on_token: len=%d", token_part.length());
#endif


    if(chosen)
    {
        chosen->on_token(token_part, last_chunk);
        return;
    }

    // NOTE: Somewhat unlikely that we'd dispatch on token itself, but conceivable
    // may want to split this into a distinct kind of dispatcher, just because dispatching on
    // token feels like an explicitly pre known circumstance i.e. session management feature
    for(int i = 0; i < handler_factory_count; i++)
    {
        // NOTE: Keeping this here in anticipation of handler_memory() floating
        // if we do the "Always"/objstack keep feature
        context_t ctx(context.incoming_context, handler_memory());

        IDispatcherHandler* handler = handler_factories[i](handler_memory());
        State& state = handler_state(i);

        if(!state.is_interested() && state.initialized()) continue;

        handler->on_token(token_part, last_chunk);

        state.interested(handler->interested());

        if(handler->is_always_interested())
        {
            chosen = handler;
            return; // obviously do NOT destruct the chosen handler
        }

        // placement new demands explicit destructor invocation
        handler->~IDispatcherHandler();
    }
}


void FactoryDispatcherHandler::on_option(number_t number, uint16_t length)
{
    if(chosen != NULLPTR)
    {
        chosen->on_option(number, length);
        return;
    }
}

void FactoryDispatcherHandler::on_option(number_t number,
                                      const pipeline::MemoryChunk::readonly_t& option_value_part,
                                      bool last_chunk)
{
    if(chosen != NULLPTR)
    {
        chosen->on_option(number, option_value_part, last_chunk);
        return;
    }

    for(int i = 0; i < handler_factory_count; i++)
    {
        context_t ctx(context.incoming_context, handler_memory());

        IDispatcherHandler* handler = handler_factories[i](handler_memory());
        State& state = handler_state(i);

        if(!state.is_interested() && state.initialized()) continue;

        // FIX: This only works for non-chunked processing
        handler->on_option(number, option_value_part.length());
        handler->on_option(number, option_value_part, true);

        state.interested(handler->interested());

        if(handler->is_always_interested())
        {
            chosen = handler;
            return; // obviously do NOT destruct the chosen handler
        }

        // placement new demands explicit destructor invocation
        handler->~IDispatcherHandler();
    }
}

void FactoryDispatcherHandler::on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                                       bool last_chunk)
{
    if(chosen != NULLPTR)
    {
        chosen->on_payload(payload_part, last_chunk);

        // NOTE: Keep an eye on this.  Not sure if this is the exact right
        // place for lifecycle management, but should be a good spot
        if(last_chunk)
            chosen->~IDispatcherHandler();

        return;
    }

    // Extremely unlikely that we won't have a chosen handler by now
    // (unlikely use case)
    for(int i = 0; i < handler_factory_count; i++)
    {
        context_t ctx(context.incoming_context, handler_memory());

        IDispatcherHandler* handler = handler_factories[i](handler_memory());
        State& state = handler_state(i);

        if(!state.is_interested() && state.initialized()) continue;

        handler->on_payload(payload_part, last_chunk);

        state.interested(handler->interested());

        if(handler->is_always_interested())
        {
            // Further unlikely that we'll dispatch mid-chunk
            // (unlikely that buffers would fall this way, in
            // addition to extremely unlikely use case)
            chosen = handler;
            return; // obviously do NOT destruct the chosen handler, though by now it doesn't much matter
        }

        // placement new demands explicit destructor invocation
        handler->~IDispatcherHandler();
    }
}


void ContextDispatcherHandler::on_header(Header header)
{
    context.header(header);
    state = header.token_length() == 0 ? 2 : 1;
}

void ContextDispatcherHandler::on_token(const pipeline::MemoryChunk::readonly_t& token_part, bool last_chunk)
{
    // FIX: access already-allocated token manager (aka token pool) and allocate a new
    // token or link to already-allocated token.
    // For now we use very unreliable static to hold this particular incoming token
    static layer2::Token token;

    // NOTE: this won't yet work with chunked
    token.set(token_part.data(), token_part.length());
    context.token(&token);
    state = 2;
}


}

}}
