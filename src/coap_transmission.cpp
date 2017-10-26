//
// Created by malachi on 10/25/17.
//

#include "coap_transmission.h"

namespace moducom {
namespace coap {

void DummyDispatcher::receive()
{

}

namespace experimental {
namespace layer2 {

/*
void option_delta_length_helper(uint16_t value, uint8_t* output)
{
    uint8_t mini_header = 0;

    if(value < CoAP::Parser::Extended8Bit)
    {
        mini_header = value << 4;
    }

} */

// TODO: make this a state machine generator also
size_t OptionGenerator::generate(uint16_t last_number, uint8_t *output)
{
    ASSERT_ERROR(true, number > last_number, "Option number not greater than preceding number");

    uint16_t delta = number - last_number;
    return 0;
}

OptionGenerator::StateMachine::StateMachine(const OptionBase& option_base) :
        option_base(option_base)
{
    _sub_state = CoAP::Parser::OptionSize;
}


uint8_t generator_helper(uint16_t value, int pos = 0)
{
    if(value < CoAP::Parser::Extended8Bit)
    {
        return value;
    }
    else if(value < COAP_EXTENDED8_BIT_MAX)
    {
        if(pos == 0) return CoAP::Parser::Extended8Bit;

        value -= 13;

        if(pos == 1) return value;
    }
    else if(value < COAP_EXTENDED16_BIT_MAX)
    {
        if(pos == 0) return CoAP::Parser::Extended16Bit;

        value -= 269;

        if(pos == 1) return (value >> 8);
        if(pos == 2) return (value & 0xFF);
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}

uint8_t OptionGenerator::StateMachine::generate()
{
    uint8_t option_delta_root = generator_helper(option_base.number - current_option_number);
    uint8_t option_length_root = generator_helper(option_base.length);

    switch(sub_state())
    {
        case CoAP::Parser::OptionSize:
            pos = 0;
            if(option_delta_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDelta);
            else if(option_length_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else
                sub_state(CoAP::Parser::OptionDeltaAndLengthDone);

            option_length_root |= option_delta_root << 4;

            return option_length_root;

        // never actually used
        case CoAP::Parser::OptionSizeDone:

        case CoAP::Parser::OptionDelta:
        {
            uint8_t option_delta_next = generator_helper(option_base.number - current_option_number, ++pos);
            if(option_delta_root == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else if(option_delta_root == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionDeltaDone);
            return option_delta_next;
        }

        case CoAP::Parser::OptionDeltaDone:
            current_option_number = option_base.number;
            sub_state(CoAP::Parser::OptionLength);

        case CoAP::Parser::OptionLength:
        {
            uint8_t option_length_next = generator_helper(option_base.length, ++pos);
            if(option_length_next == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionLengthDone);
            else if(option_length_next == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionLengthDone);
            return option_length_next;
        }

        case CoAP::Parser::OptionDeltaAndLengthDone:

        case CoAP::Parser::OptionLengthDone:
            pos = 0;
            sub_state(CoAP::Parser::OptionValue);

        case CoAP::Parser::OptionValue:
            if(pos == option_base.length)
                sub_state(CoAP::Parser::OptionValueDone);

            return option_base.value_opaque[pos++];

        case CoAP::Parser::OptionValueDone:
            break;
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}


}
}

}
}
