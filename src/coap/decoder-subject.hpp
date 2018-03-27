#pragma once

#include "decoder-subject.h"

namespace moducom { namespace coap {


// TODO: Eventually clean up dispatch_option and then
// just run process_iterate always at the bottom
template <class TMessageObserver>
bool DecoderSubjectBase<TMessageObserver>::dispatch_iterate(Decoder::Context& context)
{
    const ro_chunk_t& chunk = context.chunk;
    size_t& pos = context.pos; // how far into chunk our locus of processing should be

    switch(decoder.state())
    {
        case Decoder::HeaderDone:
            dispatch_header();
            break;

        case Decoder::TokenDone:
            dispatch_token();
            break;

        case Decoder::Options:
            dispatch_option(context);
            break;

        case Decoder::Payload:
        {
            bool last_payload_chunk = context.last_chunk;

            if(pos == 0)
                // arrives here in the unlikely case payload starts on new chunk
                // boundary
                dispatch_payload(chunk, last_payload_chunk);
            else
                // typically represents an a.1 scenario, but doesn't have to
                dispatch_payload(chunk.remainder(pos), last_payload_chunk);

            break;
        }

        default: break;
    }

    decoder.process_iterate(context);

    // TODO: Do an assert to make sure pos never is >
    ASSERT_ERROR(true, pos <= chunk.length(), "pos should never exceed chunk length");

    return pos == chunk.length();
}

template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_header()
{
    observer_on_header(decoder.header_decoder());
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


// New and improved flavor.  Compiling but not functional yet
template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_option(Decoder::Context& context)
{
    switch (decoder.option_state())
    {
        case OptionDecoder::ValueStart:
        {
            option_number_t option_number = (option_number_t) decoder.option_number();

            observer_on_option(option_number, decoder.option_length());
        }

        case OptionDecoder::OptionValue:
            observer_on_option(context.remainder());
            break;

        default:
            break;
    }
}



template <class TMessageObserver>
void DecoderSubjectBase<TMessageObserver>::dispatch_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk)
{
    observer_on_payload(payloadChunk, last_chunk);
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
