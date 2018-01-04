//
// Created by Malachi Burke on 11/25/17.
//

#include "coap-decoder.h"
#include "coap_transmission.h"

namespace moducom { namespace coap {


// Operations are done in "network byte order" according to CoAP spec, which in turn is
// big endian as suggested by https://stackoverflow.com/questions/13514614/why-is-network-byte-order-defined-to-be-big-endian
// TODO: Looks like we might be able to consolidate this a little with the generator_helper function
inline bool OptionDecoder::processOptionSize(uint8_t size_root)
{
    switch (size_root)
    {
        // first time we get here, we're already processing first byte which is also last byte,
        // so signal immediate completion
        case Extended8Bit:
            return false;

            // we wait for two bytes to come in this time
        case Extended16Bit:
            return pos != 2;

        default:
            ASSERT_ABORT(true, false, "Invalid 15 or 0-12 observed");
            return false;
    }
}


bool OptionDecoder::process_iterate(uint8_t value)
{
    // We have to determine right here if we have extended Delta and/or
    // extended Lengths
    uint8_t raw_delta = this->raw_delta();
    uint8_t raw_length = this->raw_length();

    switch (state())
    {
        // Auto-reset back to looking for size portion of header
        case OptionValueDone:
            state(FirstByte);
            return false;

        case FirstByte:
        {
            // NOTE: Not yet activated by current Dispatcher as it aborts before we reach here
            if(value == 0xFF)
            {
                // Set state to payload marker.  Also designates as processed, so this
                // implies a tiny bit of payload processing responsibility is on OptionDecoder
                state(Payload);
                return true;
            }

            buffer[0] = value;
            pos = 1;

            state(FirstByteDone);

            // NOTE: Because consumers need to inspect FirstByteDone state occasionally,
            // and importantly because cusomers ALSO need to process
            // OptionDelta/OptionDeltaDone/OptionDeltaAndLengthDone, we mark this as a NON
            // processed byte (return false).  This bends but does not break the "processed" flag
            // rule, as technically we really AREN'T done processing value byte right now.
            // NOTE: This means we could perhaps optimize OptionDecoder since caller is managing
            // the supply of the byte for the next call too
            return false;
        }

        case FirstByteDone:
        {
            bool delta_extended = raw_delta >= Extended8Bit;
            bool length_extended = raw_length >= Extended8Bit;

            if (delta_extended)
            {
                // extended delta and MAYBE extended length
                if(length_extended)
                    // then we expect to process extended delta THEN extended length
                    state(OptionDelta);
                else
                    // then we expect to announce length done, THEN process extended delta
                    state(OptionLengthDone);
            }
            else if (length_extended)
            {
                // no extended delta, but yes extended length
                // a bit of a formality, kick off OptionDeltaDone just
                // so watchers know it's OK to pull a Delta out
                // this adds overhead also during OptionDeltaDone
                // to check again on length_extended
                state(OptionDeltaDone);
            }
            else
            {
                // no extended delta or length
                // NOTE: this state might go away if we force everyone to participate in "iterative"
                // state machine behavior (since then discrete delta/length states will *always* be
                // visible)
                state(OptionDeltaAndLengthDone);
            }

            // Byte is considered fully processed NOW (see big comment in FirstByte)
            return true;
        }

        case OptionDelta:
            buffer[pos] = value;

            // NOTE: processOptionSize uses pos, so can't increment it above
            if (!processOptionSize(raw_delta))
            {
                state(OptionDeltaDone);
            }
            else
                pos++;

            return true;

        case OptionDeltaDone:
        {
            bool length_extended = raw_length >= Extended8Bit;

            if (!length_extended)
            {
                // If not an extended length flavor, then we're done processing both
                // option and delta so move to evaluate OptionValue
                state(ValueStart);
            }
            else
            {
                // otherwise, extended length, so we have to gather up length before it can
                // be acted on
                state(OptionLength);

                // FIX: would do this with an OptionLengthStart event if we had one
                pos = 1; // re-use first-byte buffer for length processing
            }

            return false;
        }

        case OptionLength:
            buffer[pos] = value;

            // NOTE: processOptionSize uses pos, so can't increment it above
            if (!processOptionSize(raw_length))
            {
                state(OptionLengthDone);
            }
            else
                pos++;

            return true;

        case OptionLengthDone:
        {
            // if length_extended is false, we arrive here BEFORE any possible extended delta
            // if length_extended is true, we arrive here AFTER any possible extended delta
            bool length_extended = raw_length >= Extended8Bit;

            if(length_extended)
                state(ValueStart);
            else
                state(OptionDelta);

            return false;

        }

        case OptionDeltaAndLengthDone:
            state(ValueStart);
            return false;

        case ValueStart:
            // NOTE: Be careful, value_length occupies same space as buffer[4]
            value_length = option_length();
            state(OptionValue);
            return false;

        case OptionValue:
            // we've completed processing this option when option_size from
            // option length has finally gone to zero
            if (--value_length == 0)
            {
                // NOTE: slight redundancy here in returning false also, perhaps optimize out
                state(OptionValueDone);
            }
            // although we don't calculate incoming value we do eat it and count it and technically
            // count it as processed, so return a true
            return true;
    }

    return true;
}

#ifdef UNUSED
bool OptionDecoder::process_iterate(pipeline::IBufferedPipelineReader& reader, OptionExperimental* built_option)
{
    pipeline::PipelineMessage msg = reader.peek();

    size_t count = 0;

    if(msg.length == 0) return false;

    while(msg.length-- > 0)
    {
        count++;
        process_iterate(*msg.data++);

        switch(state())
        {
            case OptionDeltaDone:
                built_option->number_delta += option_delta();
                break;

            case OptionLengthDone:
                built_option->length = option_length();
                // we're done at this point
                reader.advance_read(count);
                // "prematurely" set to value done, since we expect caller to manually
                // retrieve value when using the explicit reader technique
                state(OptionValueDone);
                return true;

            case OptionValue:
            case OptionValueDone:
                ASSERT_ERROR(false, true, "Should not reach here");
                break;
        }
    }


    // FIX: Build this out.  We need to be mindful of state boundaries so that
    // we can stop and let caller take action on state changes
    reader.advance_read(count);
    return false;
}
#endif

size_t OptionDecoder::process_iterate(const pipeline::MemoryChunk::readonly_t& chunk, OptionExperimental* built_option)
{
    size_t length = chunk._length(); // represents remaining length to be processed
    size_t value_processed = 0;
    const uint8_t* data = chunk.data();

    // NOTE: semi-copy paste of above iterate, for now
    // NOTE: Beef up state machine a lot, since we are putting a lot of faith into this semi-infinite-loop
    while(length > 0)
    {
        // processed represents specifically whether byte was consumed,
        // but not whether it was evaluated (bytes are always assumed to
        // be evaluated)
        bool processed = process_iterate(*data);

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
                return data - chunk.data();

                // pause here so consumer has a chance to act on a completed delta/length gather
            case ValueStart:
                return data - chunk.data();

                // remember option value processing amounts to skipping past the number of option value
                // bytes present

                // for now, also indicates completion of entire option.  Splitting out a separate OptionDone
                // state would be a little more cycling around, but better organized
            case OptionValueDone:
                return data - chunk.data();
        }
    }

    return data - chunk.data();
}


// NOTE: Not used yet by Dispatcher - but that's enroute
// ready for testing in place of ParserDeprecated
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
            while (pos < chunk.length && !process_done)
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
            while(pos < chunk.length && !process_done)
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

// handle option a.1), a.2) or b.1) described below
            if ((pos == chunk.length && last_chunk) || chunk[pos] == 0xFF)
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

        case OptionsDone:
            // TODO: Need to peer into incoming data stream to determine if a payload is on the way
            // or not - specifically need to also check size.  Here there are 3 possibilities:
            //
            // a.1) payload marker present, so process a payload, entire chunk / datagram remainder present
            // a.2) payload marker present, but only partial chunk present, so more payload to process
            // b.1) end of datagram - entire chunk present
            // b.2) end of chunk - only partial chunk present
            // c) as-yet-to-be-determined streaming end of chunk marker
            if (pos == chunk.length && last_chunk)
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
            pos = chunk.length;
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
    ASSERT_ERROR(true, pos <= chunk.length, "pos should never exceed chunk length");

    return pos == chunk.length;
}

}}
