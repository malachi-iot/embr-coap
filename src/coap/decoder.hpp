#pragma once

#include "decoder.h" // only for tooltips - decoder.hpp is auto-included by decoder.h itself (at the end)

namespace moducom { namespace coap {

template <class TDecoderTraits>
bool DecoderBase<TDecoderTraits>::header_process_iterate(internal::DecoderContext& c)
{
    bool process_done = false;

    while (c.pos < c.chunk.length() && !process_done)
    {
        process_done = header_decoder().process_iterate(c.chunk[c.pos]);

        // FIX: This meaning I believe is reversed from non decoder process calls
        // FIX: process_done does not designate whether bytes were consumed, for
        // headerDecoder and tokenDecoder, byte is always consumed
        c.pos++;
    }

    return process_done;
}

template <class TDecoderTraits>
bool DecoderBase<TDecoderTraits>::token_process_iterate(internal::DecoderContext& c)
{
    bool process_done = false;

    while(c.pos < c.chunk.length() && !process_done)
    {
        // TODO: Utilize a simpler counter and chunk out token
        process_done = token_decoder().process_iterate(c.chunk[c.pos], header_decoder().token_length());

        // FIX: This meaning I believe is reversed from non decoder process calls
        // FIX: process_done does not designate whether bytes were consumed, for
        // headerDecoder and tokenDecoder, byte is always consumed
        c.pos++;
    }

    return process_done;
}



}}
