#include "coap.h"
#include "memory.h"

namespace moducom {
namespace coap {


// Operations are done in "network byte order" according to CoAP spec, which in turn is
// big endian as suggested by https://stackoverflow.com/questions/13514614/why-is-network-byte-order-defined-to-be-big-endian
bool CoAP::Parser::processOptionSize(uint8_t size_root)
{
    if (size_root < Extended8Bit)
    {
        option_size = size_root;
        return false;
        // Just use literal value, not extended
        //return nonextended;
    }
    else if (size_root == Extended8Bit)
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
        ASSERT_ABORT(true, false, "Invalid 15 observed")
        return false;
    }
}

bool CoAP::Parser::processOption(uint8_t value)
{
    // Designates payload follows
    if (value == 0xFF)
    {
        return false;
    }

    // we don't process OptionValue, just skip over it (outer code processes
    // option values)
    if(sub_state() != OptionValue) buffer[pos++] = value;

    switch (sub_state())
    {
        case OptionValueDone:
            sub_state(OptionSize);

        case OptionSize:
            sub_state(OptionSizeDone);
            break;

        case OptionSizeDone:
            sub_state(OptionDelta);

        case OptionDelta:
            if (!processOptionSize((buffer[0] & 0xF0) >> 4))
            {
                // Done with Delta
                // strip off no-longer used Delta size so that Length is
                // easier to process
                buffer[0] &= 0xF0;
                sub_state(OptionDeltaDone);
                // TODO: do something with option_size here
                pos = 1; // re-use buffer for length processing
            }
            break;

        case OptionDeltaDone:
            sub_state(OptionLength);

        case OptionLength:
            if (!processOptionSize(buffer[0]))
            {
                sub_state(OptionLengthDone);
                // TODO: do something with option_size here
            }
            break;

        case OptionLengthDone:
            sub_state(OptionValue);

        case OptionValue:
            // we've completed processing this option when option_size from
            // option length has finally gone to zero
            if (--option_size == 0)
            {
                // NOTE: slight redundancy here in returning false also, perhaps optimize out
                sub_state(OptionValueDone);
                return false;
            }
            break;
    }

    //if(pos++ > 3)

    return true;
}

void CoAP::Parser::process(uint8_t value)
{
    switch (state)
    {
        case Header:
            buffer[pos++] = value;

            if (pos == 4)
            {
                state = HeaderDone;
                pos = 0;
            }
            break;

        case HeaderDone:
            state = Options;
            sub_state(OptionSize);

        case Options:
            // remember, we could be processing multiple options here
            if ((sub_state() == OptionValueDone || sub_state() == OptionSize)
                && value == 0xFF)
                state = OptionsDone;
            else
                processOption(value);

            break;

        case OptionsDone:
            break;
    }
}


CoAP::Parser::Parser()
{
    state = Header;
    pos = 0;
}

void CoAP::ParseToIResponder::process(uint8_t message[], size_t length)
{
    uint16_t option_number = 0;
    uint16_t option_length;
    uint8_t* option_value;

    for (int i = 0; i < length; i++)
    {
        Parser::State state = parser.get_state();

        if (state != Parser::Payload)
        {
            // processing header + options
            parser.process(message[i]);

            switch (state = parser.get_state())
            {
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
                        case Parser::OptionDeltaDone:
                        {
                            uint16_t option_delta = parser.get_option_size();
                            option_number += option_delta;
                            break;
                        }

                        case Parser::OptionLengthDone:
                            option_length = parser.get_option_size();
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