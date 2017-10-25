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
    if(sub_state != OptionValue) buffer[pos++] = value;

    switch (sub_state)
    {
        case OptionSize:
            break;

        case OptionDelta:
            if (!processOptionSize((buffer[0] & 0xF0) >> 4))
            {
                // Done with Delta
                // strip off no-longer used Delta size so that Length is
                // easier to process
                buffer[0] &= 0xF0;
                sub_state = OptionLength;
                // TODO: do something with option_size here
                pos = 1; // re-use buffer for length processing
            }
            break;

        case OptionLength:
            if (!processOptionSize(buffer[0]))
            {
                sub_state = OptionValue;
                // TODO: do something with option_size here
            }
            break;

        case OptionValue:
            // we've completed processing this option when option_size from
            // option length has finally gone to zero
            if (--option_size == 0) return false;
            break;
    }

    //if(pos++ > 3)

    return true;
}

void CoAP::Parser::process(uint8_t value)
{
    nonPayLoadSize++;

    switch (state)
    {
        case Header:
            buffer[pos++] = value;

            header <<= 8;
            header |= value;

            if (pos == 4)
            {
                // TODO: this could be optimized further
                uint16_t message_id = COAP_UINT16_FROM_NBYTES(buffer[2], buffer[3]);
                // create our header
                state = Options;
                pos = 0;
            }
            break;

        case Options:
            if (!processOption(value))
                state = Payload;

            break;

        case Payload:
            break;
    }
}


CoAP::Parser::Parser()
{
    state = Header;
    pos = 0;
    //low_level_callback = NULLPTR;
    nonPayLoadSize = 0;
    header = 0;
}

}
}