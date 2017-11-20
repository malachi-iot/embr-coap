#include "coap.h"
#include "mc/memory.h"

namespace moducom {
namespace coap {


// Operations are done in "network byte order" according to CoAP spec, which in turn is
// big endian as suggested by https://stackoverflow.com/questions/13514614/why-is-network-byte-order-defined-to-be-big-endian
// TODO: Looks like we might be able to consolidate this a little with the generator_helper function
bool CoAP::Parser::processOptionSize(uint8_t size_root)
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
    /*
    if (size_root < Extended8Bit)
    {
        option_size = size_root;
        return false;
        // Just use literal value, not extended
        //return nonextended;
    }
    else 
    if (size_root == Extended8Bit)
    {
        option_size = buffer[pos] + 13;
        return false;
    }
    else if (size_root == Extended16Bit)
    {
        switch(pos)
        {
            // First byte (big endian)
            case 1:
                option_size = (uint16_t) buffer[pos] << 8;
                return true;

            // Second byte (big endian)
            case 2:
                option_size |= buffer[pos];
                option_size += 269;
                return false;
        }
    }
    else // RESERVED
    {
        ASSERT_ABORT(true, false, "Invalid 15 or 0-12 observed")
        return false;
    } */
}

bool CoAP::Parser::processOption(uint8_t value)
{
    // We have to determine right here if we have extended Delta and/or
    // extended Lengths
    uint8_t raw_delta = this->raw_delta();
    uint8_t raw_length = this->raw_length();

    switch (sub_state())
    {
        case OptionValueDone:
            sub_state(OptionSize);
            return false;

        case OptionSize:
        {
            buffer[0] = value;
            pos = 1;

            sub_state(OptionSizeDone);

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
                sub_state(OptionDelta);
            }
            else if (length_extended)
            {
                // no extended delta, but yes extended length
                // a bit of a formality, kick off OptionDeltaDone just
                // so watchers know it's OK to pull a Delta out
                // this adds overhead also during OptionDeltaDone
                // to check again on length_extended
                sub_state(OptionDeltaDone);
            }
            else
                // no extended delta or length
                // NOTE: this state might go away if we force everyone to participate in "iterative"
                // state machine behavior (since then discrete delta/length states will *always* be
                // visible)
                sub_state(OptionDeltaAndLengthDone);

            // FIX: eventually this should return a *false* as reflected in 
            // OptionSize comments
            return true;
        }

        case OptionDelta:
            buffer[pos] = value;

            // NOTE: processOptionSize uses pos, so can't increment it above
            if (!processOptionSize(raw_delta))
            {
                sub_state(OptionDeltaDone);
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
                sub_state(OptionLengthDone);
            }
            else
            {
                // otherwise, extended length, so we have to gather up length before it can
                // be acted on
                sub_state(OptionLength);

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
                sub_state(OptionLengthDone);
            }
            else
                pos++;

            return true;

        case OptionDeltaAndLengthDone:
        case OptionLengthDone:
            // NOTE: Be careful, option_size occupies same space as buffer[4]
            option_size = option_length();
            sub_state(OptionValue);
            return false;

        case OptionValue:
            // we've completed processing this option when option_size from
            // option length has finally gone to zero
            if (--option_size == 0)
            {
                // NOTE: slight redundancy here in returning false also, perhaps optimize out
                sub_state(OptionValueDone);
            }
            return true;
    }

    return true;
}

bool CoAP::Parser::process_iterate(uint8_t value)
{
    switch (_state)
    {
        case Header:
            buffer[pos++] = value;

            if (pos == 4) _state = HeaderDone;
            return true;

        case HeaderDone:
        {
            // HeaderDone clearly we have header available
            CoAP::Header* header = (CoAP::Header*) this->header();

            if(header->token_length() > 0)
            {
                state(Token);
                // we're going to be tricky and store this
                // in a union with sub_state, so that
                // we don't allocate a new variable space
                _token_length = header->token_length();
            }
            else
            {

                // Set up for Options start
                _state = Options;
                sub_state(OptionSize);
            }

            return false;
        }

        case Token:
        {
            buffer[pos++] = value;
            if(pos == token_length())
                state(TokenDone);

            return true;
        }

        case TokenDone:
        {
            // Set up for Options start
            _state = Options;
            sub_state(OptionSize);

            return false;
        }

        case Options:
            // remember, we could be processing multiple options here
            // or, we could encounter 0 options with only
            // a payload
            if (sub_state() == OptionSize && value == 0xFF)
            {
                // A little clumsy - want OptionsDone to represent
                // when we naturally run out of bytes - not get an 0xFF
                // though it stands to reason a state machine can never
                // count on knowing options done because 0xFF marker may or
                // may not appear, and if it does, it's ALWAYS a payload
                _state = Payload;
                return true;
            }
            else
                return processOption(value);

        // all options are done being processed, and we actually shouldn't get here
        // since this is a header+option handling area only
        case OptionsDone:
        case Payload:
            ASSERT_ERROR(false, false, "Should not be here");
            break;
    }

    return true;
}


CoAP::Parser::Parser()
{
    _state = Header;
    pos = 0;
}

void CoAP::ParseToIResponder::process(const uint8_t message[], size_t length)
{
    uint16_t option_number = 0;
    uint16_t option_length;
    const uint8_t* option_value;

    for (int i = 0; i < length; i++)
    {
        Parser::State state = parser.state();

        // FIX: Need something a little clearer, since we get this status when we
        // are end-of-message that never had a payload also
        if (state != Parser::Payload)
        {
            bool processed;

            do
            {
                // processing header + options
                processed = parser.process_iterate(message[i]);

                switch (parser.state())
                {
                    case Parser::Header:
                        break;

                    case Parser::HeaderDone:
                    {
                        // we *just* finished processing header, so extract it
                        Header* header = (Header*) parser.header();

                        responder->OnHeader(*header);
                        break;
                    }

                    case Parser::TokenDone:
                    {
                        responder->OnToken(parser.token(), parser.token_length());
                        break;
                    }

                    case Parser::Options:
                    {
                        Parser::SubState sub_state = parser.sub_state();
                        switch (sub_state)
                        {
                            case Parser::OptionSize:
                            case Parser::OptionSizeDone:
                            case Parser::OptionDelta:
                                // we see these, but don't care about them
                                break;

                            case Parser::OptionDeltaAndLengthDone:
                                option_length = parser.option_length();
                                // FIX: This is glitchy, because we reach this status *before*
                                // consuming byte, therefore we're off by one (because OptionSize
                                // is not consuming byte as it should)
                                option_value = &message[i + 1];

                            case Parser::OptionDeltaDone:
                            {
                                uint16_t option_delta = parser.option_delta();
                                option_number += option_delta;
                                break;
                            }

                            case Parser::OptionLengthDone:
                                option_length = parser.option_length();
                                option_value = &message[i];
                                break;

                            case Parser::OptionValueDone:
                            {
                                OptionExperimental option(option_number, option_length, option_value);
                                responder->OnOption(option);
                                break;
                            }
                        }
                        break;
                    }

                    // effectively same as payload ready
                    case Parser::OptionsDone:
                        // parser.state(Parser::Payload);
                        // FIX: brute force into Payload immediately so we don't waste time
                        // processing and ignoring another character
                        // also it will trigger an assert, since we're not supposed to process OptionsDone in there
                        break;

                    case Parser::Payload:
                        // Should see this once and only once

                        // i + 1 because payload content starts one after this payload marker
                        ASSERT_ERROR(true, length - (i + 1) > 0, "Payload marker seen but no payload found");
                        break;
                }
            } while (!processed);
        }
        else
        {
            // payload processing happens here
            responder->OnPayload(&message[i], length - i);
            responder->OnCompleted();
            return;
        }
    }

    responder->OnCompleted();
}


// FIX: Not ready!  Does not account for incoming chunks which *ARENT* the whole message
// TODO: there's a special way the multi-part coap messages work which I forgot, read
// up on that before actually trying to tackle contiguous CoAP messages
bool CoAP::ParseIterateToIResponder::process_iterate(const pipeline::MemoryChunk& incoming)
{
    option_state_t& option = state.option;
    uint8_t* message = incoming.data;
    size_t length = incoming.length;

    for (int i = 0; i < length; i++)
    {
        Parser::State state = parser.state();

        // FIX: Need something a little clearer, since we get this status when we
        // are end-of-message that never had a payload also
        if (state != Parser::Payload)
        {
            bool processed;

            do
            {
                // processing header + options
                processed = parser.process_iterate(message[i]);

                switch (parser.state())
                {
                    case Parser::Header:
                        break;

                    case Parser::HeaderDone:
                    {
                        // we *just* finished processing header, so extract it
                        Header* header = (Header*) parser.header();

                        responder->OnHeader(*header);
                        break;
                    }

                    case Parser::TokenDone:
                    {
                        responder->OnToken(parser.token(), parser.token_length());
                        break;
                    }

                    case Parser::Options:
                    {
                        Parser::SubState sub_state = parser.sub_state();
                        switch (sub_state)
                        {
                            case Parser::OptionSize:
                            case Parser::OptionSizeDone:
                            case Parser::OptionDelta:
                                // we see these, but don't care about them
                                break;

                            case Parser::OptionDeltaAndLengthDone:
                                option.length = parser.option_length();
                                // FIX: only do this if chunk actually has entire
                                // option present
                                // FIX: This is glitchy, because we reach this status *before*
                                // consuming byte, therefore we're off by one (because OptionSize
                                // is not consuming byte as it should)
                                // FIX: rough idea, but needs fine tuning
                                if(length - i > option.length)
                                {
                                    option.value = &message[i + 1];
                                }
                                else
                                {
                                    // TBD: how to handle boundary-span
                                }

                            case Parser::OptionDeltaDone:
                            {
                                uint16_t option_delta = parser.option_delta();
                                option.number += option_delta;
                                break;
                            }

                            case Parser::OptionLengthDone:
                                // FIX: only do this if chunk actually has entire
                                // option present
                                option.length = parser.option_length();
                                // FIX: rough idea, but needs fine tuning
                                if(length - i > option.length)
                                {
                                    option.value = &message[i];
                                }
                                else
                                {
                                    // TBD: how to handle boundary-span
                                }
                                break;

                            case Parser::OptionValueDone:
                            {
                                OptionExperimental option(option.number, option.length, option.value);
                                responder->OnOption(option);
                                break;
                            }
                        }
                        break;
                    }

                        // effectively same as payload ready
                    case Parser::OptionsDone:
                        // parser.state(Parser::Payload);
                        // FIX: brute force into Payload immediately so we don't waste time
                        // processing and ignoring another character
                        // also it will trigger an assert, since we're not supposed to process OptionsDone in there
                        break;

                    case Parser::Payload:
                        // Should see this once and only once

                        // i + 1 because payload content starts one after this payload marker
                        ASSERT_ERROR(true, length - (i + 1) > 0, "Payload marker seen but no payload found");
                        break;
                }
            } while (!processed);
        }
        else
        {
            // FIX: payload sort of handles chunking already, but this call
            // will still probably need attention
            // payload processing happens here
            responder->OnPayload(&message[i], length - i);
            responder->OnCompleted();
            return false;
        }
    }

    return false;
}

}
}
