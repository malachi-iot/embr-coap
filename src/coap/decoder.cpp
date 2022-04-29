/**
 * @file
 * @author  Malachi Burke
 */

#include "decoder.h"
#include "../coap_transmission.h"

namespace embr { namespace coap {

Decoder::ro_chunk_t Decoder::option(Context& context, bool* completed)
{
    ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

    // assert that we are at ValueStart or OptionValue (latter when chunked)
    ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at ValueStart");

    // NOTE: Safe to grab this, option_decoder().option_length() doesn't get clobbered for a while still
    int value_length = option_decoder().option_length();
    ro_chunk_t ret = context.remainder();
    bool _completed = value_length <= ret.size();

    if(completed != NULLPTR)
        *completed = _completed;
    else
    {
        ASSERT_WARN(true, _completed, "Partial data encountered but potentially ignored");
    }

    // if completed, be sure we resize down the remainder to a maximum
    // value_length size rather than the entire remaining buffer
    if(_completed)   ret.resize(value_length);

    return ret;
}


iterated::decode_result Decoder::process_iterate(Context& context)
{
    size_t& pos = context.pos;
    bool last_chunk = context.last_chunk;
    //typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef Context::chunk_t ro_chunk_t;
    const ro_chunk_t& chunk = context.chunk;

    iterated::decode_result r{false, false, false, false};

    switch (state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            state(Header);
            init_header_decoder();
            break;

        case Header:
        {
            bool process_done = decoder_base_t::header_process_iterate(context);

            if (process_done) state(HeaderDone);

            break;
        }

        case HeaderDone:
            if(header_decoder().token_length() > 0)
            {
                state(TokenStart);
            }
            else
                // We used to skip this and go direct to OptionsStart, which is not a terrible idea
                // but debatable.  However, due to this quote from RFC7252:
                // "(Note that every message carries a token, even if it is of zero length.)"
                // we are definitely going to indicate TokenDone even with an empty token
                state(TokenDone);
            break;

        case TokenStart:
            // Being that we initialize token decoder just after reaching TokenStart,
            // tkl in header_decoder() is still there until we get here, if we want it
            state(Token);
            init_token_decoder();
            break;

        case Token:
        {
            bool process_done = decoder_base_t::token_process_iterate(context);

            if(process_done) state(TokenDone);

            break;
        }

        case TokenDone:
            state(OptionsStart);
            break;

        // OptionsStart is always reached.  We determine here if we
        // need to process any options
        case OptionsStart:
        {
            ro_chunk_t remainder = context.remainder();

            // if we're at EOF (happens with header+token only messages)
            if(remainder.size() == 0 && last_chunk)
            {
                // then never go through an options processing phase at all
                state(OptionsDone);
            }
            else
            {
                init_option_decoder();
                optionHolder.reset();

                // We have to do some level of processing on OptionsStart to know
                // whether we have any optons at all.  So remember
                pos += option_decoder().process_iterate(remainder, &optionHolder, last_chunk);
                // hit payload immediately (no options, just a payload marker presumably
                // followed by a payload)
                if (option_state() == OptionDecoder::Payload)
                    // we'll continue to handle payload marker within OptionsDone
                    state(OptionsDone);
                else
                    state(Options);
            }
            break;
        }

        case Options:
        {
            pos += option_decoder().process_iterate(context.remainder(), &optionHolder, last_chunk);

            // handle option a.1), a.2) or b.1) described below
            if (option_state() == OptionDecoder::Payload)
            {
                // we'll continue to handle payload marker within OptionsDone
                state(OptionsDone);
            }
            // reach here under circumstance where there is no payload at all -
            // we're at the end of the buffer and no payload marker seen
            else if ((pos == chunk.size() && last_chunk))
            {
                // now that optionDecoder.process_iterate can iterate at EOF with no characters,
                // this is as simple as waiting until we get OptionValueDone
                if(option_state() == OptionDecoder::OptionValueDone)
                    state(OptionsDone);
            }

            break;
        }

        case OptionsDone:
            // Encounters this when we lead with a payload, eventually all payload markers
            // I want handled this way so we can eliminate the explicit 0xFF later
            if(option_state() == OptionDecoder::Payload)
            {
                state(Payload);
            } else
            // TODO: Need to peer into incoming data stream to determine if a payload is on the way
            // or not - specifically need to also check size.  Here there are 3 possibilities:
            //
            // a.1) payload marker present, so process a payload, entire chunk / datagram remainder present
            // a.2) payload marker present, but only partial chunk present, so more payload to process
            // b.1) end of datagram - entire chunk present
            // b.2) end of chunk - only partial chunk present
            // c) as-yet-to-be-determined streaming end of chunk marker
            if (pos == chunk.size() && last_chunk)
            {
                // this is for condition b.1)
                state(Done);
            }
            else if (chunk[pos] == 0xFF)
            {
                // FIX: This code should no longer be called, but it still getting activated by
                // Dispatcher.  Address this with new DecoderSubject class, leave dispatcher alone
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
            // fast forward pos to end of chunk since here on out it
            // only contains payload information
            pos = chunk.size();
            if(last_chunk)  state(PayloadDone);
            break;

        case PayloadDone:
            state(Done);
            break;

        case Done:
            r.done = true;
            state(Uninitialized);
            break;

    }

    // TODO: Do an assert to make sure pos never exceeds chunk boundary
    ASSERT_ERROR(true, pos <= chunk.size(), "pos should never exceed chunk length");

    if(last_chunk)
        r.eof = pos == chunk.size();
    else
        r.waitstate = pos == chunk.size();

    return r;
}

}}
