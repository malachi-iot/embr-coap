#include "coap.h"
#include "memory.h"

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
            //pos = 0;

        case OptionSize:
        {
            buffer[0] = value;
            pos = 1;

            // FIX: have to reassign this, because raw_delta & friends
            // get their values from buffer[0]
            raw_delta = this->raw_delta();
            raw_length = this->raw_length();
            bool delta_extended = raw_delta >= Extended8Bit;
            bool length_extended = raw_length >= Extended8Bit;

            if (!delta_extended && !length_extended)
                // Done with length AND delta processing
                sub_state(OptionDeltaAndLengthDone);
            else if (length_extended)
            {
                // then it's only length extended, so move there
                sub_state(OptionDeltaDone);
            }
            else // only other possibility is only delta extended
            {
                sub_state(OptionSizeDone);
            }
            break;
        }

        case OptionSizeDone:
            sub_state(OptionDelta);
            return false;

        case OptionDelta:
            buffer[pos] = value;

            // NOTE: processOptionSize uses pos, so can't increment it above
            if (!processOptionSize(raw_delta))
            {
                sub_state(OptionDeltaDone);

                // FIX: would do this with an OptionLengthStart event if we had one
                pos = 1; // re-use buffer for length processing
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
                // otherwise, extended length, so we have to gather up length before it can
                // be acted on
                sub_state(OptionLength);

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
                // FIX: a little hacky, have to do this here so that our buffer[pos++] doesn't wig out
                pos = 0;
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
            break;

        case HeaderDone:
        {
            // Set up for Options start
            _state = Options;
            sub_state(OptionSize);
            pos = 0;
            // Fall thru to options, as most "Done"s should do
        }

        case Options:
            // remember, we could be processing multiple options here
            if ((sub_state() == OptionValueDone || sub_state() == OptionSize)
                && value == 0xFF)
            {
                // A little clumsy - want OptionsDone to represent
                // when we naturally run out of bytes - not get an 0xFF
                _state = Payload;
                return true;
            }
            else
                return processOption(value);

            break;

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

void CoAP::ParseToIResponder::process(uint8_t message[], size_t length)
{
    uint16_t option_number = 0;
    uint16_t option_length;
    uint8_t* option_value;

    for (int i = 0; i < length; i++)
    {
        Parser::State state = parser.state();

        if (state != Parser::Payload)
        {
            // processing header + options
            parser.process(message[i]);

            switch (parser.state())
            {
                case Parser::Header:
                    break;

                case Parser::HeaderDone:
                {
                    // we *just* finished processing header, so extract it
                    Header* header = (Header*)parser.get_header();

                    responder->OnHeader(*header);
                    break;
                }

                case Parser::Options:
                    switch (parser.sub_state())
                    {
                        case Parser::OptionSize:
                        case Parser::OptionSizeDone:
                        case Parser::OptionDelta:
                            // we see these, but don't care about them
                            break;

                        case Parser::OptionDeltaAndLengthDone:
                            option_length = parser.option_length();
                            option_value = &message[i + 1];

                        case Parser::OptionDeltaDone:
                        {
                            uint16_t option_delta = parser.option_delta();
                            option_number += option_delta;
                            break;
                        }

                        case Parser::OptionLengthDone:
                            option_length = parser.option_length();
                            option_value = &message[i + 1];
                            break;

                        case Parser::OptionValueDone:
                        {
                            OptionExperimental option(option_number, option_length, option_value);
                            responder->OnOption(option);
                            break;
                        }
                    }
                    break;

                // effectively same as payload ready
                case Parser::OptionsDone:
                    // parser.state(Parser::Payload);
                    // FIX: brute force into Payload immediately so we don't waste time
                    // processing and ignoring another character
                    // also it will trigger an assert, since we're not supposed to process OptionsDone in there
                    break;

                case Parser::Payload:
                    // Should see this once and only once
                    //ASSERT_ERROR(false, false, "Should not be here");
                    break;
            }
        }
        else
        {
            // payload processing happens here
            responder->OnPayload(&message[i], length - i);
            return;
        }
    }
}

}
}
