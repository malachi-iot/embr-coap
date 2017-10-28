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
    current_option_number = 0;
    pos = 0;
}


uint8_t generator_helper(uint16_t value, int pos = 0)
{
    if (value < CoAP::Parser::Extended8Bit)
    {
        return value;
    }
    else if (value < COAP_EXTENDED8_BIT_MAX)
    {
        if (pos == 0) return CoAP::Parser::Extended8Bit;

        value -= 13;

        if (pos == 1) return value;
    }
    else if (value < COAP_EXTENDED16_BIT_MAX)
    {
        if (pos == 0) return CoAP::Parser::Extended16Bit;

        value -= 269;

        if (pos == 1) return (value >> 8);
        if (pos == 2) return (value & 0xFF);
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}

// TODO: make all these ins and outs a different type
// so that we can spit out -1 or similar indicating "more processing required"
// so that we don't have to pull stunts to pop around extra steps
OptionGenerator::StateMachine::output_t OptionGenerator::StateMachine::generate_iterate()
{
    uint8_t option_delta_root = generator_helper(option_base.number - current_option_number);
    uint8_t option_length_root = generator_helper(option_base.length);

    switch (sub_state())
    {
        case CoAP::Parser::OptionSize:
            pos = 0;
            sub_state(CoAP::Parser::OptionSizeDone);

            option_length_root |= option_delta_root << 4;

            return option_length_root;

        case CoAP::Parser::OptionSizeDone:
            if (option_delta_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDelta);
            else if (option_length_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else
                sub_state(CoAP::Parser::OptionDeltaAndLengthDone);

            return signal_continue;

        case CoAP::Parser::OptionDelta:
        {
            uint8_t option_delta_next = generator_helper(option_base.number - current_option_number, ++pos);
            if (option_delta_root == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else if (option_delta_root == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionDeltaDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_delta_next;
        }

        case CoAP::Parser::OptionDeltaDone:
            current_option_number = option_base.number;
            if (option_length_root >= CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionLength);
            else
                sub_state(CoAP::Parser::OptionLengthDone);

            return signal_continue;

        case CoAP::Parser::OptionLength:
        {
            uint8_t option_length_next = generator_helper(option_base.length, ++pos);
            if (option_length_next == CoAP::Parser::Extended8Bit)
                sub_state(CoAP::Parser::OptionLengthDone);
            else if (option_length_next == CoAP::Parser::Extended16Bit && pos == 2)
                sub_state(CoAP::Parser::OptionLengthDone);
            else if (pos > 2)
            {
                ASSERT_ERROR(false, false, "Should not be here");
            }
            return option_length_next;
        }

        case CoAP::Parser::OptionDeltaAndLengthDone:

        case CoAP::Parser::OptionLengthDone:
            pos = 0;
            sub_state(CoAP::Parser::OptionValue);

        case CoAP::Parser::OptionValue:
            if (pos == option_base.length - 1)
                sub_state(CoAP::Parser::OptionValueDone);

            return option_base.value_opaque[pos++];

        case CoAP::Parser::OptionValueDone:
            break;
    }

    ASSERT_ERROR(false, false, "Should not get here");
    return -1;
}


}

void TestResponder::OnHeader(const coap::CoAP::Header header)
{
}

void TestResponder::OnOption(const coap::CoAP::OptionExperimental& option)
{
#ifdef __CPP11__
    typedef CoAP::Header::TypeEnum type_t;
#else
    typedef CoAP::Header type_t;
#endif

    // figure out if we're handling a request or a response
    switch(header.type())
    {
        // request
        case type_t::Confirmable:
        case type_t::NonConfirmable:
            OnOptionRequest(option);
            break;

            // response
        case type_t::Acknowledgement:
            OnOptionResponse(option);
            break;
    }
}

void TestResponder::OnOptionRequest(const coap::CoAP::OptionExperimental& option)
{
#ifdef __CPP11__
    typedef coap::CoAP::OptionExperimental::Numbers enum_t;
#else
    typedef coap::CoAP::OptionExperimental enum_t;
#endif    

    switch (option.get_number())
    {
        case enum_t::UriPath:
        {
            // TODO: make it switchable whether we allocate more memory or keep pointing
            // to the value buffer given to us
            std::string uri((const char*)option.value, option.length);

            // RFC7252 Section 6.5 #6
            this->uri += uri + "/";

            // TODO: case sensitivity flag

#ifdef DEBUG
            char temp[128];
            this->uri.populate(temp);
#endif
            break;
        }

        case enum_t::ContentFormat:
            break;

        case enum_t::UriPort:
        {
            port = option.value_uint;
            break;
        }
    }

    // we have enough to assign user_responder, so do so
    // FIX: be sure to pay attention to port as well
    if (user_responder == NULLPTR && option.number > enum_t::UriPath)
    {
        user_responder = uri_list[this->uri];
    }

    if (user_responder)
    {
        user_responder->OnOption(option);
    }
}



void TestResponder::OnOptionResponse(const CoAP::OptionExperimental &option)
{
    header.token_length();
}


void TestResponder::OnPayload(const uint8_t message[], size_t length)
{
    // FIX: consolidate/clean this code
    if (user_responder == NULLPTR)
    {
        user_responder = uri_list[uri];

        if(user_responder == NULLPTR) return;
    }

    user_responder->OnPayload(message, length);
}

}
}
}
