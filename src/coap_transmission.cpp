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

uint8_t OptionGenerator::StateMachine::generate()
{
    switch(sub_state())
    {
        case CoAP::Parser::OptionSize:
            return 0;
    }
}


}
}

}
}