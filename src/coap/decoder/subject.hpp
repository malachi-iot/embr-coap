#pragma once

#include "subject.h"

namespace moducom { namespace coap {


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
            observer_on_payload(context.remainder(), context.last_chunk);

            // FIX: A little hacky, falls through to brute force on_complete
            // we *might* run into double-calls of Decoder::Done in this case
            if(!context.last_chunk) break;

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
            observer_on_option(context.remainder());
            break;

        default:
            break;
    }
}



template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_token()
{
    const TokenDecoder& token_decoder = decoder.token_decoder();
    // NOTE: 90% sure it's safe to intermingle header decoder + token decoder (pretty
    // sure we retain header all the way thru) but doublecheck and document
    ro_chunk_t chunk(token_decoder.data(),
                        decoder.header_decoder().token_length());

    // FIX: TODO: Chunking out token eventually will be necessary
    observer_on_token(chunk, true);
}

}}
