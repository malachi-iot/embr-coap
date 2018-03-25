//
// Created by Malachi Burke on 11/25/17.
//

#include "coap-decoder.h"
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
                state(OptionsStart);
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
            state(Options);
            break;

        case Options:
        {
            pos += optionDecoder.process_iterate(chunk.remainder(pos), &optionHolder);

            // FIX: Payload now discovered and heeded in optionDecoder, do so out here
            // as well
// handle option a.1), a.2) or b.1) described below
            if (optionDecoder.state() == OptionDecoder::Payload)
            {
                // FIX: Clunky and not working
            }
            else if ((pos == chunk.length() && last_chunk) || chunk[pos] == 0xFF)
            {
                // OptionsValueDone = processing one option, and reached the end of the entire option
                // Payload = never even processed an option, but instead hit payload marker immediately
                //           [this needs work, as Payload marker technically can appear even when actual
                //            options were encountered]
                // OptionDeltaAndLengthDone = TODO: not sure why we allow this here, seems like a partial state thing
                //                            perhaps to accomodate chunking
                ASSERT_ERROR(true,
                             (optionDecoder.state() == OptionDecoder::OptionValueDone) ||
                             (optionDecoder.state() == OptionDecoder::Payload) ||
                             (optionDecoder.state() == OptionDecoder::OptionDeltaAndLengthDone),
                             "Must be either optionValueDone or optionDeltaAndlengthDone.  Got: " << optionDecoder.state());
                // will check again for 0xFF if necessary
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
            if (pos == chunk.length() && last_chunk)
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

    // TODO: Do an assert to make sure pos never is >
    ASSERT_ERROR(true, pos <= chunk.length(), "pos should never exceed chunk length");

    return pos == chunk.length();
}

}}
