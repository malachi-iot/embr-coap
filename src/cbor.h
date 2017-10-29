#ifndef CBOR_H
#define CBOR_H

#include <stdint.h>

#include "platform.h"

namespace moducom {

class CBOR
{
public:
    enum Types
    {
        UnsignedInteger = 0,
        PositiveInteger = 0,
        NegativeInteger = 1,
        ByteArray = 2,
        String = 3,
        ItemArray = 4,
        Map = 5,
        Tag = 6,
        Float = 7,
        SimpleData = 7
    };

    class DecoderStateMachine
    {
        union
        {
            // TODO: consider taking first byte out so that count union goes more smoothly
            uint8_t buffer[9];

            // TODO: make this into a 64 bit counter
            uint16_t count;
        };

        uint8_t pos;

        enum State
        {
            MajorType,
            MajorTypeDone,
            AdditionalInteger,
            AdditionalIntegerDone,
            ByteArrayState,
            ByteArrayDone,
            ItemArrayState,
            ItemArrayDone
        };

        enum AdditionalIntegerInformation
        {
            // all self contained
            bits_none = 0,

            bits_8 = 24,
            bits_16 = 25,
            bits_32 = 26,
            bits_64 = 27
        };

        State _state;

        uint8_t additional_integer_information() const
        {
            return buffer[0] & 0x1F;
        }

        bool has_additional_integer_information() const
        {
            switch(additional_integer_information())
            {
                bits_8:
                bits_16:
                bits_32:
                bits_64:
                    return true;

                default:
                    return false;
            }
        }

        Types type() const { return (Types) (buffer[0] >> 5); }

        void state(State _state) { this->_state = _state; }

        uint8_t get_value_8() const { return buffer[1]; }
        uint16_t get_value_16() const
        {
            uint16_t value = buffer[1];

            value <<= 8;
            value |= buffer[2];
        }

    public:
        bool process_iterate(uint8_t value);

        void process(uint8_t value)
        {
            while (!process_iterate(value));
        }

        State state() const { return _state; }

    };


    class EncoderStateMachine
    {

    };
};

}

#endif // CBOR_H
