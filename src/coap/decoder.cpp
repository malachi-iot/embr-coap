//
// Created by Malachi Burke on 11/25/17.
//

#include "decoder.h"
#include "coap_transmission.h"

namespace moducom { namespace coap {

bool Decoder::process_iterate(Context& context)
{
    size_t& pos = context.pos;
    bool process_done = false;
    bool last_chunk = context.last_chunk;
    const pipeline::MemoryChunk::readonly_t& chunk = context.chunk;

    switch (state())
    {
        case Uninitialized:
            // If necessary, initialize header decoder
            state(Header);
            init_header_decoder();
            break;

        case Header:
        {
            while (pos < chunk.length() && !process_done)
            {
                process_done = header_decoder().process_iterate(chunk[pos]);

                // FIX: This meaning I believe is reversed from non decoder process calls
                // FIX: process_done does not designate whether bytes were consumed, for
                // headerDecoder and tokenDecoder, byte is always consumed
                pos++;
            }

            if (process_done) state(HeaderDone);

            break;
        }

        case HeaderDone:
            if(header_decoder().token_length() > 0)
            {
                state(Token);
                // TODO: May want a TokenStart state
                init_token_decoder();
            }
            else
                // We used to skip this and go direct to OptionsStart, which is not a terrible idea
                // but debatable.  However, due to this quote from RFC7252:
                // "(Note that every message carries a token, even if it is of zero length.)"
                // we are definitely going to indicate TokenDone even with an empty token
                state(TokenDone);
            break;

        case Token:
            while(pos < chunk.length() && !process_done)
            {
                // TODO: Utilize a simpler counter and chunk out token
                process_done = token_decoder().process_iterate(chunk[pos], header_decoder().token_length());

                // FIX: This meaning I believe is reversed from non decoder process calls
                // FIX: process_done does not designate whether bytes were consumed, for
                // headerDecoder and tokenDecoder, byte is always consumed
                pos++;
            }

            if(process_done) state(TokenDone);

            break;

        case TokenDone:
            state(OptionsStart);
            break;

        case OptionsStart:
            init_option_decoder();
            optionHolder.number_delta = 0;
            optionHolder.length = 0;
            // We have to do some level of processing on OptionsStart to know
            // whether we have any optons at all.  So remember
            pos += optionDecoder.process_iterate(chunk.remainder(pos), &optionHolder);
            if(optionDecoder.state() == OptionDecoder::Payload)
                state(OptionsDone);
            else
            {
                state(Options);

                /*
                // Dispatcher/DecoderSubject don't like this , but eventually should
                // To prep, have Dispatcher/DecoderSubject look for 'ValueStart' rather
                // than "option header" end condition

                // If we *do* have options and we aren't done processing "option header"
                if(optionDecoder.state() != OptionDecoder::OptionDeltaAndLengthDone)
                {
                    // And we have more chunk data available,
                    // go ahead and proceed to iterate a bit more through the options
                    if (pos < chunk.length()) return process_iterate(context);
                }
                // we don't want to push forward if we're at OptionsDeltaAndLengthDone
                // since we want to give consumers a chance to react to that
                */
            }
            break;

        case Options:
        {
            pos += optionDecoder.process_iterate(chunk.remainder(pos), &optionHolder);

            // FIX: Payload now discovered and heeded in optionDecoder, do so out here
            // as well
// handle option a.1), a.2) or b.1) described below
            if (optionDecoder.state() == OptionDecoder::Payload)
            {
                state(OptionsDone);
            }
            // reach here under circumstance where there is no payload at all -
            // we're at the end of the buffer and no payload marker seen
            else if ((pos == chunk.length() && last_chunk))
            {
                // OptionsValueDone = processing one option, and reached the end of the entire option
                // Payload = never even processed an option, but instead hit payload marker immediately
                //           [this needs work, as Payload marker technically can appear even when actual
                //            options were encountered]
                // OptionDeltaAndLengthDone = TODO: not sure why we allow this here, seems like a partial state thing
                //                            perhaps to accomodate chunking
                ASSERT_ERROR(true,
                             (optionDecoder.state() == OptionDecoder::OptionValueDone) ||
                             (optionDecoder.state() == OptionDecoder::OptionDeltaAndLengthDone),
                             "Must be either optionValueDone or optionDeltaAndlengthDone.  Got: " << optionDecoder.state());
                // will check again for 0xFF if necessary
                state(OptionsDone);
            }
            // reach here when we are not at last chunk but still in mid-options processing
            else
            {
                //ASSERT_ERROR(false, optionDecoder.state() == OptionDecoder::OptionValueDone, "dummy test");
                // UNSUPPORTED
                // b.2 means we are not sure if another option or an incoming chunk[0] == 0xFF is coming
                // MAY need an OptionsBoundary state, but hopefully not
                // better would be to beef up OptionDecoder awareness of Payload marker
            }

            break;
        }

        case OptionsDone:
            // Encounters this when we lead with a payload, eventually all payload markers
            // I want handled this way so we can eliminate the explicit 0xFF later
            if(optionDecoder.state() == OptionDecoder::Payload)
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
            if (pos == chunk.length() && last_chunk)
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
            // fast forward pos to end of chunk since chunk here on out chunk
            // only contains payload information
            pos = chunk.length();
            if(last_chunk)  state(PayloadDone);
            break;

        case PayloadDone:
            state(Done);
            break;

        case Done:
            state(Uninitialized);
            break;

    }

    // TODO: Do an assert to make sure pos never exceeds chunk boundary
    ASSERT_ERROR(true, pos <= chunk.length(), "pos should never exceed chunk length");

    return pos == chunk.length();
}

}}
