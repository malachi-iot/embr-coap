#include "coap.h"
#include "memory.h"

namespace moducom {
namespace coap {

// Operations are done in "network byte order" according to CoAP spec, which in turn is
// big endian as suggested by https://stackoverflow.com/questions/13514614/why-is-network-byte-order-defined-to-be-big-endian
bool CoAP::Parser::processOptionSize(uint8_t size_root, uint8_t local_position)
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
        switch(local_position)
        {
            // First byte (big endian)
            case 0:
                option_size = (uint16_t) buffer[pos] << 8;
                return true;

            case 1:
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

    buffer[pos++] = value;

    switch (sub_state)
    {
        case OptionSize:
            // just record and skip by this one, use it later
            local_position = 0;
            break;

        case OptionDelta:
            if (processOptionSize(buffer[0] & 0xF0 >> 4, local_position))
                local_position++;
            else
            {
                // Done with Delta
                // strip off no-longer used Delta size so that Length is
                // easier to process
                buffer[0] &= 0xF0;
                sub_state = OptionLength;
                /*
                // TODO: do something with option_size here
                if(low_level_callback)
                {
                    low_level_callback(callback_context);
                } */
            }
            break;

        case OptionLength:
            if(processOptionSize(buffer[0], local_position))
                local_position++;
            else
            {
                // TODO: do something with option_size here
                return false;
            }
            break;
    }

    //if(pos++ > 3)
}

void CoAP::Parser::process(uint8_t value)
{
    nonPayLoadSize++;

    switch (state)
    {
        case Header:
            buffer[pos] = value;
            if (pos++ == 4)
            {
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
}

}
}