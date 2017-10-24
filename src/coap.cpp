#include "coap.h"

namespace moducom {
namespace coap {

bool CoAP::Parser::processOptionSize(uint8_t size_root, uint8_t value, uint8_t local_position)
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
        //(*index_bump)++;
        //return 13 + *extended;
    }
    else if (size_root == Extended16Bit)
    {
        // FIX: BEWARE of endian issue!!
        //uint16_t _extended = *((uint16_t*)extended);

        //(*index_bump) += 2;

        //return 269 + _extended;
    }
    else // RESERVED
    {

    }
}

bool CoAP::Parser::processOption(uint8_t value)
{
    if (value == 0xFF)
    {
        return false;
    }

    buffer[pos] = value;

    switch (sub_state)
    {
        case OptionSize:
            // just record and skip by this one, use it later
            break;

        case OptionDelta:
            if (!processOptionSize(buffer[0] & 0xF0 >> 4, value, pos))
            {
                // when done, reset pos so that we keep looking
                //pos = 1;
                // TODO: need two pos at once, local and "global"
            }
            break;

        case OptionLength:
            processOptionSize(buffer[0] & 0x0F, value, -1);
            break;
    }

    //if(pos++ > 3)
}

void CoAP::Parser::process(uint8_t value)
{
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
}

}
}