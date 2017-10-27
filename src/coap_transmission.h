//
// Created by malachi on 10/25/17.
//

#ifndef SRC_COAP_TRANSMISSION_H
#define SRC_COAP_TRANSMISSION_H

#include "platform.h"
#include "coap.h"
#include "string.h"

namespace moducom {
namespace coap {

namespace experimental {
namespace layer2 {

class OptionValue
{
protected:
public:
    union
    {
        const char*     value_string;
        const uint8_t*  value_opaque;
        const uint16_t  value_uint;
    };

    uint16_t length;
};

class OptionString : public OptionValue
{
public:
    const char* value() { return value_string; }
};


class OptionOpaque : public OptionValue
{
public:
    const uint8_t* value() { return value_opaque; }
    const uint16_t length() { return OptionValue::length; }
};


class OptionNumber
{
protected:
public:
    const uint16_t number;

    OptionNumber(uint16_t number) : number(number) {}
};

class OptionBase : public OptionValue,
                        public OptionNumber
{
public:
    OptionBase(uint16_t number) : OptionNumber(number) {}
};

class OptionGenerator : public OptionBase
{
public:
    class StateMachine
    {
        uint16_t current_option_number;
        uint8_t pos;
        const OptionBase& option_base;
        CoAP::Parser::SubState _sub_state;

        void sub_state(CoAP::Parser::SubState _sub_state)
        {
            this->_sub_state = _sub_state;
        }

    public:
        StateMachine(const OptionBase&);

        typedef int16_t output_t;

        static CONSTEXPR output_t signal_continue = -1;

        output_t generate_iterate();
        uint8_t generate()
        {
            output_t result;
            do
            {
                result = generate_iterate();
            } while(result == signal_continue);

            return result;
        }

        CoAP::Parser::SubState sub_state() const { return _sub_state; }
    };

public:
    OptionGenerator(uint8_t number) : OptionBase(number) {}

    //! Create option in-place in output
    //! \param output
    //! \return
    size_t generate(uint16_t last_number, uint8_t* output);
};


class OptionUriHost : public OptionString,
                      public OptionNumber
{
public:
    OptionUriHost(char* value) : OptionNumber(CoAP::OptionExperimental::UriHost)
    {
        value_string = value;
    }

    void validate(void* validator);
};


class CoapMessage
{
public:
};


}
namespace layer5 {

class OptionValue
{
    virtual void copy_to(uint8_t* destination) = 0;
};

class OptionNumber
{
    virtual void validate(void* validator, const OptionValue& value) = 0;
};

class OptionBase
{
public:
    const uint8_t number;

    OptionBase(uint8_t number) : number(number) {}

    virtual void populate(uint8_t* destination) = 0;
    virtual void validate(void* validator) = 0;
};

class OptionStringNative : public OptionBase
{
public:
    OptionStringNative(uint8_t number) : OptionBase(number) {}
};

// pure data entity for conveniently holding an outgoing option
class OptionString : public OptionBase
{
public:
    OptionString(uint8_t number) :
            OptionBase(number) {}

    moducom::std::string value;
};


class OptionOpaque : public OptionBase
{
public:
    OptionOpaque(uint8_t number) : OptionBase(number) {}

    moducom::dynamic::MemoryBuffer* value;
};


class OptionUInt : public OptionBase
{
public:
    OptionUInt(uint8_t number) : OptionBase(number) {}

    uint16_t value;
};


class OptionIfMatch : public OptionOpaque
{
public:
    OptionIfMatch() : OptionOpaque(CoAP::OptionExperimental::IfMatch) {}
};


class OptionUriHost : public OptionString
{
public:
    OptionUriHost() : OptionString(CoAP::OptionExperimental::UriHost) {}
};

// pure data entity for conveniently holding an outgoing coap message
class CoapResponse
{
public:
    void add(const OptionBase& option);
};

}
}


namespace layer2 {

class CoapSender
{
public:
    //void prepare_for_send(experimental::layer2::OptionValue)
};

}


// blocking auto dispatcher for all send/receive operations
class CoapDispatcher
{
protected:
    //virtual uint8_t* get_upcoming_send_buffer(size_t& requested_size);
    //virtual uint8_t* get_receive_buffer(size_t* received_size);
    virtual void receive() = 0;
public:

};


class DummyDispatcher : public CoapDispatcher
{
protected:
    virtual void receive() OVERRIDE;
};

}

namespace coap {
namespace experimental
{

class TestResponder : coap::CoAP::IResponder
{
public:
    virtual void OnHeader(const coap::CoAP::Header header) OVERRIDE;
    virtual void OnOption(const coap::CoAP::OptionExperimental& option) OVERRIDE;
    virtual void OnPayload(const uint8_t message[], size_t length) OVERRIDE;
};

}
}

}


#endif //SRC_COAP_TRANSMISSION_H
