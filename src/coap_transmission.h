//
// Created by malachi on 10/25/17.
//

#ifndef SRC_COAP_TRANSMISSION_H
#define SRC_COAP_TRANSMISSION_H

//#include <mc/mem/platform.h>
//#include <mc/memory.h>
#include "coap.h"

// interim: eventually provide our own memory pool version of map
#include <map>

namespace embr {
namespace coap {

namespace experimental {
namespace layer2 {

class OptionLength
{
public:
    uint16_t length;
};

// Raw data for CoAP value.  Encoding/decoding happens elsewhere
class OptionValue //: public OptionLength
{
protected:
public:
    union
    {
        // DEBT: Phase out value_string also as that too is a bit of
        // encode/decode, though much simpler than the int processing
        const char*     value_string;
        const uint8_t*  value_opaque;
        // We don't attempt a encode/decode of CoAP int here anymore
        //const uint16_t  value_uint;
    };

    // C++98 requirement
    //OptionValue() : value_uint(0) {}

    OptionValue() : value_opaque(NULLPTR) {}
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
    //uint16_t length() { return OptionValue::length; }
};


class OptionNumber
{
protected:
public:
    const uint16_t number;

    OptionNumber(uint16_t number) : number(number) {}
};


class OptionHeader :
        public OptionLength,
        public OptionNumber
{
public:
    OptionHeader(uint16_t number) : OptionNumber(number) {}
};

class OptionBase :
        public OptionHeader,
        public OptionValue
{
public:
    OptionBase(uint16_t number) : OptionHeader(number) {}
};

class OptionUriHost : public OptionString,
                      public OptionNumber
{
public:
    OptionUriHost(char* value) : OptionNumber(Option::UriHost)
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
}


}

}


#endif //SRC_COAP_TRANSMISSION_H
