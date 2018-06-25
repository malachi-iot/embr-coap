#pragma once

#include "subject.h"

namespace moducom { namespace coap {

namespace experimental {

// shall bear very strong resemblace to predecessor's DecoderSubjectBase dispatch_iterate
template <class TSubject>
bool notify_from_decoder(TSubject& subject, Decoder& decoder, Decoder::Context& context)
{
    typedef event_base::buffer_t buffer_t;

    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            subject.notify(header_event { decoder.header_decoder() });
            break;

        case Decoder::TokenDone:
        {
            // FIX: Not 100% sure token_length is available at this point
            buffer_t chunk(decoder.token_decoder().data(),
                             decoder.header_decoder().token_length());

            subject.notify(token_event(chunk, true));
            break;
        }

        case Decoder::Options:
            switch(decoder.option_state())
            {
                // a lot of option states pass by, but we always latch on to the ValueStart state
                // remember, this state is seen even with "0-length" values
                case OptionDecoder::ValueStart:
                {
                    uint16_t option_number = decoder.option_number();

                    if (decoder.option_length() > 0)
                    {
                        bool completed;

                        // will take more work than commented code, and not as fast,
                        // but this way it's code reuse & dogfooding
                        buffer_t b = decoder.option(context, &completed);
                        subject.notify(option_event(option_number, b, completed));
                    }
                    else
                        subject.notify(option_event(option_number));
                }

                default: break;
            }
            break;

        case Decoder::Payload:
        {
            subject.notify(payload_event(context.remainder(), true));

            // FIX: A little hacky, falls through to brute force on_complete
            // we *might* run into double-calls of Decoder::Done in this case
            if(!context.last_chunk) break;
        }

        case Decoder::Done:
            subject.notify(completed_event { });
            break;

        default: break;
    }

    return decoder.process_iterate(context);
}

}

// TODO: Eventually clean up dispatch_option and then
// just run process_iterate always at the bottom
template <class TMessageObserver>
bool DecoderSubjectBase<TMessageObserver>::dispatch_iterate(Decoder::Context& context)
{
    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            observer_on_header(decoder.header_decoder());
            break;

        case Decoder::TokenDone:
            dispatch_token();
            break;

        case Decoder::Options:
            dispatch_option(context);
            break;

        case Decoder::Payload:
        {
            observer_on_payload(context.remainder_legacy(), context.last_chunk);

            // FIX: A little hacky, falls through to brute force on_complete
            // we *might* run into double-calls of Decoder::Done in this case
            if(!context.last_chunk) break;
        }

        case Decoder::Done:
            // FIX: Frequently not called, needs work -
            // DecoderSubjectBase needs to know
            // incoming message length (is exhausted) to really ascertain this
            // That said, incoming message chunks denote 'last chunk'
            // indicating whether to expect more data, that should
            // be enough to deduce a done state
            observer_on_complete();
            break;

        default: break;
    }

    return decoder.process_iterate(context);
}


template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::observer_on_option(const ro_chunk_t& chunk)
{
    option_number_t option_number = (option_number_t) decoder.option_number();
    uint16_t option_length = decoder.option_length();

    if(chunk.length() < option_length)
    {
        observer_on_option(option_number, chunk, false);
        // We expect at least 1 more call to observer_on_option,
        // so decrement length accordingly
        decoder.optionHolder.length -= chunk.length();
    }
    else
    {
        observer_on_option(option_number, chunk.subset(option_length), true);
    }
}


// during times where we are processing options at the Decoder level, handle
// incoming "option header" and option-value data
template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_option(Decoder::Context& context)
{
    switch (decoder.option_state())
    {
        case OptionDecoder::ValueStart:
        {
            option_number_t option_number = (option_number_t) decoder.option_number();
            uint16_t option_length = decoder.option_length();

            observer_on_option(option_number, option_length);

            if(option_length == 0) break;
        }

        case OptionDecoder::OptionValue:
            observer_on_option(context.remainder_legacy());
            break;

        default:
            break;
    }
}



template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_token()
{
    // Though we like to observe TokenDone messages, we don't pass that on
    // as an empty token observation.  This is already indicated during header
    // of tkl==0
    if(decoder.header_decoder().token_length() == 0) return;

    const Decoder::token_decoder_t& token_decoder = decoder.token_decoder();
    // NOTE: 90% sure it's safe to intermingle header decoder + token decoder (pretty
    // sure we retain header all the way thru) but doublecheck and document
    ro_chunk_t chunk(token_decoder.data(),
                        decoder.header_decoder().token_length());

    // FIX: TODO: Chunking out token eventually will be necessary
    observer_on_token(chunk, true);
}

}}
