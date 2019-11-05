#include "streambuf.h"
#include "option.h"

namespace moducom { namespace coap { namespace experimental {

template <class TStreambuf>
bool StreambufDecoder<TStreambuf>::process_iterate_streambuf()
{
    // NOTE: pos' days are numbered, only using this for compatibility with non streambuf decoder
    size_t& pos = context.pos;

    // FIX: this is going to glitch when in_avail reports '-1' when we are expecting
    // '0'
    estd::streamsize in_avail = streambuf.in_avail();
    // TODO: use underflow to determine last_chunk rather than total_size_remaining
    bool last_chunk = total_size_remaining <= in_avail;

    switch (state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            state(Header);
            init_header_decoder();
            break;

        case Header:
        {
            bool process_done = false;

            uint8_t ch;

            while(!process_done &&
                (ch = streambuf.sbumpc()) != traits_type::eof())
                process_done = header_decoder().process_iterate(ch);

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
            bool process_done = false;

            uint8_t ch;
            uint8_t tkl = header_decoder().token_length();

            while(!process_done &&
                  (ch = streambuf.sbumpc()) != traits_type::eof())
                process_done = token_decoder().process_iterate(ch, tkl);

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
            // if we're at EOF (happens with header+token only messages)
            //if(remainder.size() == 0 && last_chunk)
            if(in_avail <= 0 && last_chunk)
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
                int count = option_decoder().process_iterate_streambuf(streambuf, &optionHolder);

                pos += count;

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
            int count = option_decoder().process_iterate_streambuf(streambuf, &optionHolder);

            pos += count;

            // FIX: this is going to glitch when in_avail reports '-1' when we are expecting
            // '0'
            in_avail = streambuf.in_avail();
            // NOTE: yes, somewhat kludgey reassigning last_chunk here
            last_chunk = total_size_remaining <= in_avail;

            // handle option a.1), a.2) or b.1) described below
            if (option_state() == OptionDecoder::Payload)
            {
                // we'll continue to handle payload marker within OptionsDone
                state(OptionsDone);
            }
                // reach here under circumstance where there is no payload at all -
                // we're at the end of the buffer and no payload marker seen
            else if (in_avail <= 0 && last_chunk)
            {
                // now that optionDecoder.process_iterate can iterate at EOF with no characters,
                // this is as simple as waiting until we get OptionValueDone
                if(option_state() == OptionDecoder::OptionValueDone)
                    state(OptionsDone);
            }

            break;
        }

        case OptionsDone:
        {
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
            if (in_avail <= 0 && last_chunk)
            {
                // this is for condition b.1)
                state(Done);
            }
            else if (streambuf.sgetc() == COAP_PAYLOAD_MARKER)
            {
                // FIX: This code should no longer be called, but it still getting activated by
                // Dispatcher.  Address this with new DecoderSubject class, leave dispatcher alone
                pos++;
                streambuf.sbumpc(); // toss the character

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
        }

        case Payload:
            // fast forward pos to end of chunk since here on out it
            // only contains payload information
            // NOTE: this only works for payloads which don't span chunks
            // NOTE: this only matters for compatibility with non-streambuf-decoder, which is only temporary
            pos += in_avail;
            if(last_chunk)  state(PayloadDone);
            break;

        case PayloadDone:
            state(Done);
            break;

        case Done:
            state(Uninitialized);
            break;

    }

    in_avail = streambuf.in_avail();

    // if we have indeterminate characters left or no characters left
    if(in_avail <= 0)
    {
        // underflow to grab the next buffer, and if that comes back eof
        if(streambuf.underflow() == traits_type::eof())
            // then return 'true' signifying end of data
            return true;
    }
    return false;
    //return pos == chunk.size();
}

}

// EXPERIMENTAL
// copied from OptionDecoder::process_iterate
template <class TStreambuf>
size_t OptionDecoder::process_iterate_streambuf(TStreambuf& streambuf, OptionExperimental* built_option)
{
    size_t length = streambuf.in_avail(); // represents remaining length to be processed
    int underflow = streambuf.underflow();  // only valid if length <= 0
    bool last_chunk = underflow == -1;

    // last chunk + length == 0 means special EOF processing mode
    if(length <= 0 && last_chunk)
    {
        process_iterate(0, true);
        return 0;
    }

    estd::const_buffer chunk((const uint8_t*) streambuf.gptr(), length);

    int count = process_iterate(chunk, built_option);

    // basically ignore/move past those processed characters
    // FIX: in_span_streambuf doesn't have a working pubseekoff yet
    streambuf.pubseekoff(count, estd::ios_base::cur);

    return count;

    // --- was experimenting with a full copy/paste but I think calling upon existing const_buffer code is better
    // though there's an awkwardness still with 'last_chunk', though quite diminished

    //size_t value_processed = 0;
    const uint8_t* const data_start = (const uint8_t*) streambuf.gptr();
    const uint8_t* data = data_start;

    // NOTE: semi-copy paste of above iterate, for now
    // NOTE: Beef up state machine a lot, since we are putting a lot of faith into this semi-infinite-loop
    while(length > 0)
    {
        // processed represents specifically whether byte was consumed,
        // but not whether it was evaluated (bytes are always assumed to
        // be evaluated)
        // FIX: pass proper eof flag in here
        bool processed = process_iterate(*data, false);

        if(processed)
        {
            data++;
            length--;
        }

        // take special actions during particular states during chunk processing
        switch(state())
        {
            case OptionDeltaDone:
                built_option->number_delta += option_delta();
                break;

            case OptionDeltaAndLengthDone:
                built_option->number_delta += option_delta();

            case OptionLengthDone:
                built_option->length = option_length();
                // we stop here, since caller will likely want to take prepatory action
                // now that option number/delta and option length are available
                return data - data_start;

                // pause here so consumer has a chance to act on a completed delta/length gather
            case ValueStart:
                return data - data_start;

                // remember option value processing amounts to skipping past the number of option value
                // bytes present

                // for now, also indicates completion of entire option.  Splitting out a separate OptionDone
                // state would be a little more cycling around, but better organized
            case OptionValueDone:
                return data - data_start;

            case Payload:
                return data - data_start;

                // OptionValue gets eaten here, but data position
                // does move forward and is ultimately recorded
            default: break;
        }
    }

    return data - data_start;
}


}}