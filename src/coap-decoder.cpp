//
// Created by Malachi Burke on 11/25/17.
//

#include "coap-decoder.h"

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
        case OptionValueDone:
            state(OptionSize);
            return false;

        case OptionSize:
        {
            buffer[0] = value;
            pos = 1;

            state(OptionSizeDone);

            // FIX: normally we'd return true here since we have "processed" the value
            // but we want to reveal whether we want to report
            // OptionDelta, OptionDeltaDone or OptionDeltaAndLengthDone
            // once we stabalize the "iterative" state machine behavior more hopefully this
            // will shake out and we can revert this back to false
            return false;
        }

        case OptionSizeDone:
        {
            bool delta_extended = raw_delta >= Extended8Bit;
            bool length_extended = raw_length >= Extended8Bit;

            if (delta_extended)
            {
                // extended delta and MAYBE extended length
                state(OptionDelta);
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
                // no extended delta or length
                // NOTE: this state might go away if we force everyone to participate in "iterative"
                // state machine behavior (since then discrete delta/length states will *always* be
                // visible)
                state(OptionDeltaAndLengthDone);

            // FIX: eventually this should return a *false* as reflected in
            // OptionSize comments
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
                // If not an extended length flavor, move right away to OptionLengthDone
                // and return so that consumer has a change to act on it
                state(OptionLengthDone);
            }
            else
            {
                // otherwise, extended length, so we have to gather up length before it can
                // be acted on
                state(OptionLength);

                // FIX: would do this with an OptionLengthStart event if we had one
                pos = 1; // re-use buffer for length processing
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

        case OptionDeltaAndLengthDone:
        case OptionLengthDone:
            // NOTE: Be careful, option_size occupies same space as buffer[4]
            option_size = option_length();
            state(OptionValue);
            return false;

        case OptionValue:
            // we've completed processing this option when option_size from
            // option length has finally gone to zero
            if (--option_size == 0)
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


bool OptionDecoder::process_iterate(pipeline::IBufferedPipelineReader& reader)
{
    pipeline::PipelineMessage msg = reader.peek();

    if(msg.length == 0) return false;

    // FIX: Build this out.  We need to be mindful of state boundaries so that
    // we can stop and let caller take action on state changes
    return false;
}


}}