#ifndef CBOR_H
#define CBOR_H

#include <stdint.h>

#include "platform.h"

//#define CBOR_FEATURE_32_BIT
//#define CBOR_FEATURE_64_BIT

#ifdef CBOR_FEATURE_64_BIT
#ifndef CBOR_FEATURE_32_BIT
#define CBOR_FEATURE_32_BIT
#endif
#endif

namespace moducom {

// https://tools.ietf.org/html/rfc7049
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

    enum SimpleTypes
    {
        Boolean,
        Null
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

        DecoderStateMachine* _nested;

        void alloc_nested() {}

        DecoderStateMachine* lock_nested() { return _nested; }
        void free_nested() {}
        void unlock_nested() {}

    public:
        enum State
        {
            MajorType,
            MajorTypeDone,
            AdditionalInteger,
            AdditionalIntegerDone,
            ByteArrayState,
            ByteArrayDone,
            ItemArrayState,
            ItemArrayDone,
            MapState,
            MapDone,
            Done,
            Pop // if we are nested, pop one level
        };

    private:

        enum AdditionalIntegerInformation
        {
            // all self contained
            bits_none = 0,

            bits_8 = 24,
            bits_16 = 25,
            bits_32 = 26,
            bits_64 = 27,
            Reserved1 = 28,
            Reserved2 = 30,
            Indefinite = 31
        };

        State _state;

        uint8_t additional_integer_information() const
        {
            return buffer[0] & 0x1F;
        }

        bool is_indefinite() const
        {
            return additional_integer_information() == Indefinite;
        }

        // for use during indefinite processing.  Note that the nested decoder
        // discovers this and the parent queries the nested one for it on a pop
        // condition
        bool encountered_break() const
        {
            return buffer[0] == 0xFF;
        }

        void assign_additional_integer_information();

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

        void state(State _state) { this->_state = _state; }

        uint8_t get_value_8() const { return buffer[1]; }
        uint16_t get_value_16() const
        {
            uint16_t value = buffer[1];

            value <<= 8;
            value |= buffer[2];

            return value;
        }

        bool process_iterate_nested(uint8_t value, bool* encountered_break);

        // rfc7409 section 2.3
        uint8_t get_simple_value() const { return buffer[0] & 0x1F; }

        // rfc7409 section 2.3
        bool is_simple_value() const
        {
            return type() == SimpleData && get_simple_value() <= 24;
        }

        bool is_simple_type_bool() const
        {
            uint8_t simple_value = get_simple_value();

            return type() == SimpleData && (simple_value == 20 || simple_value == 21);
        }


        bool simple_value_bool() const
        {
            ASSERT_ERROR(true, is_simple_type_bool(), "Not a true/false type");

            return get_simple_value() == 21;
        }


        bool is_simple_value_null() const
        {
            ASSERT_ERROR(true, is_simple_value(), "Not a simple value");

            return get_simple_value() == 22;
        }



    public:
        DecoderStateMachine() : _nested(NULLPTR) {}

        Types type() const { return (Types) (buffer[0] >> 5); }

        bool process_iterate(uint8_t value);

        void process(uint8_t value)
        {
            while (!process_iterate(value));
        }

        State state() const { return _state; }

    };

    class IDecoderObserver
    {
    public:
        virtual void OnTag(const DecoderStateMachine& decoder) = 0;

        // when this is fired, the integer/count information is all resolved also
        virtual void OnType(const DecoderStateMachine& decoder) = 0;

        // fired potentially multipled time for byte array, string, item array
        virtual void OnArray(const DecoderStateMachine& decoder) = 0;

        // fired potentially multiple times for map
        virtual void OnMap(const DecoderStateMachine& decoder) = 0;

        virtual void OnFloat(const DecoderStateMachine& decoder) = 0;

        virtual void OnCompleted() = 0;
    };


    class EncoderStateMachine
    {

    };


    class DecodeToObserver
    {
    public:
        IDecoderObserver* observer;

        void decode(const uint8_t* cbor, size_t len);
    };
};

}

#endif // CBOR_H
