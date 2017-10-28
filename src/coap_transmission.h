//
// Created by malachi on 10/25/17.
//

#ifndef SRC_COAP_TRANSMISSION_H
#define SRC_COAP_TRANSMISSION_H

#include "platform.h"
#include "coap.h"
#include "string.h"

// interim: eventually provide our own memory pool version of map 
#include <map>


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
    
    // C++98 requirement
    OptionValue() : value_uint(0) {}
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
struct Token
{
    uint8_t data[8];
    uint8_t length;

    CoAP::IResponder* responder;
};

// TODO: Just a fledgling class, will need to be a lot more powerful
// shared token manager so that request and response logic can be aware of each
// other's tokens
class TokenManager
{
    Token tokens[8];

public:
    TokenManager()
    {
    }

    void add(const uint8_t* token, uint8_t length, CoAP::IResponder* responder)
    {
        Token& t = tokens[0];

        t.length = length;
        memcpy(t.data, token, length);
        t.responder = responder;
    }

    const Token* get(const uint8_t* token, uint8_t length) const
    {
        for(int i = 0 ; i < 8; i++)
        {
            const Token& t = tokens[i];

            if(t.length == length && memcmp(t.data, token, length) == 0)
            {
                return &t;
            }
        }
    }
};


// Note there 2 scenarios for memory handled from the one calling IResponder:
//
// 1. memory only lasts as long as the OnXXX call (streaming)
// 2. memory only lasts as long as all the OnXXX calls (datagram/buffer based)
// 
// we'll want two very similar versions of this responder for those two scenarios
// note that even if it SEEMS like option #2, we might want to process it like option #1
// so that we can multithread it and dump/reuse the datagram buffer before we're fully
// done processing
class TestResponder : public CoAP::IResponder
{
    CoAP::IResponder* user_responder;
    TokenManager token_manager;

    std::string uri;
    uint16_t port;
    // NOTE: probably can optimize this out
    CoAP::Header header;

    typedef bool(*comp_fn)(std::string s1, std::string s2);

    // TODO: Try to get std::string& into here
    static bool str_cmp(std::string s1, std::string s2)
    {
        bool result = s1 < s2; // TODO: do proper < compare here, add that to our std::string
        return result;
    }

    // Hmm there's no specific equality version... so how do we find stuff?
    //typedef ::std::map<std::string, CoAP::IResponder*, comp_fn> map_t;
    typedef ::std::map<std::string, CoAP::IResponder*> map_t;

    // put THIS into a map of ports, otherwise the key must get more complicated and include port
    map_t uri_list;

    void OnOptionRequest(const CoAP::OptionExperimental& option);
    void OnOptionResponse(const CoAP::OptionExperimental& option);

public:
    virtual void OnHeader(const CoAP::Header header) OVERRIDE;
    virtual void OnToken(const uint8_t message[], size_t length) OVERRIDE;
    virtual void OnOption(const CoAP::OptionExperimental& option) OVERRIDE;
    virtual void OnPayload(const uint8_t message[], size_t length) OVERRIDE;

    TestResponder() :
        user_responder(NULLPTR)
        //uri_list(str_cmp)
    {}

    // Doesn't handle port yet
    void add_handle(const std::string& uri, CoAP::IResponder* ur)
    {
        uri_list[uri] = ur;
    }
};

}
}

}


#endif //SRC_COAP_TRANSMISSION_H
