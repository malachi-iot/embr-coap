#ifndef CBOR_H
#define CBOR_H

#include <stdint.h>

//#include <mc/mem/platform.h>
#include "cbor/features.h"
// TODO: Change naming and decouple UInt helpers from coap
#include "coap/uint.h"
// TODO: Liberate state helper from coap-decoder so that we aren't dependent on it
#include "coap/decoder.h"

#ifdef CBOR_FEATURE_64_BIT
#ifndef CBOR_FEATURE_32_BIT
#define CBOR_FEATURE_32_BIT
#endif
#endif


namespace embr {

// https://tools.ietf.org/html/rfc7049
//
//  0
//  0 1 2 3 4 5 6 7
// +---------------+
// |typ| addn'l int|
// +---------------+
//
// FEATURE_MCCBOR_NESTED not an available feature, and without it, CBOR decoder mainly just reports
// decoding of Major Value and Additional Integer Data.  Low-level interaction with CBOR still required,
// namely manual skipping of either additional-value indicated lengths, indeterminate lengths, or map
// which does not reveal direct byte counts
// Notion is a smarter decoder/parser will wrap around/extend it to manage hierarchical positioning
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

    class Decoder
    {
        typedef coap::internal::const_buffer const_buffer;

        union
        {
            // TODO: consider taking first byte out so that count union goes more smoothly
#ifdef CBOR_FEATURE_64_BIT
            uint8_t buffer[9];
#else
            uint8_t buffer[5];
#endif

#ifdef CBOR_FEATURE_64_BIT
            uint64_t count;
#elif defined(CBOR_FEATURE_32_BIT)
            uint32_t count;
#else
            uint16_t count;
#endif
        };

        uint8_t pos;

#ifdef FEATURE_MCCBOR_NESTED

        Decoder* _nested;

        void alloc_nested() {}

        Decoder* lock_nested() { return _nested; }
        void free_nested() {}
        void unlock_nested() {}
#endif

    public:
        enum State
        {
            Uninitialized,
            MajorType,
            MajorTypeDone,
            AdditionalInteger,
            AdditionalIntegerDone,
#ifdef FEATURE_MCCBOR_NESTED
            ByteArrayState,
            ByteArrayDone,
            ItemArrayState,
            ItemArrayDone,
            MapState,
            MapDone,
#endif
            // Indicates a particular MajorType AND AdditionalInteger (if any) are done
            Done,
            Pop // if we are nested, pop one level (only used for FEATURE_MCCBOR_NESTED)
        };

        typedef State state_t;

        // Used during non-simple-data processing
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

    private:

        State _state;

        uint8_t additional_integer_information() const
        {
            return buffer[0] & 0x1F;
        }

        uint8_t additional_integer_information_wordsize() const
        {
            switch(additional_integer_information())
            {
                case bits_8: return 1;
                case bits_16: return 2;
                case bits_32: return 4;
                case bits_64: return 8;

                default:
#ifdef DEBUG
                    // TODO: Extra diagnostic reporting for invalid state
#endif
                    return -1;
            }
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
                case bits_8:
                case bits_16:
                case bits_32:
                case bits_64:
                    return true;

                default:
                    return false;
            }
        }

        void state(State _state) { this->_state = _state; }

        uint8_t get_value_8() const { return buffer[1]; }
        uint16_t get_value_16() const
        {
            return coap::UInt::get<uint16_t>(&buffer[1]);

            /*
            uint16_t value = buffer[1];

            value <<= 8;
            value |= buffer[2];

            return value; */
        }

        // retrieve value only from additional value spot, use for more performant
        // scenarios.  Otherwise, call regular value()
        template <typename T>
        T value_additional() const
        {
            uint8_t incoming_wordsize = additional_integer_information_wordsize();

            ASSERT_ERROR(true, sizeof(T) >= incoming_wordsize, "presented word size not large enough");

            return coap::UInt::get<T>(&buffer[1], incoming_wordsize);
        }

    public:
        bool is_8bit() const { return additional_integer_information() == bits_8; }
        bool is_16bit() const { return additional_integer_information() == bits_16; }
        bool is_32bit() const { return additional_integer_information() == bits_32; }
        bool is_64bit() const { return additional_integer_information() == bits_64; }

        template <typename T>
        bool is_bitness() const { return false; }

        // FIX: Should work, but a bit dangerous
        template <typename T>
        T value() const
        {
            if(get_simple_value() <= 24) return get_simple_value();

            return value_additional<T>();
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
            Types t = type();

            return t == SimpleData && (simple_value == 20 || simple_value == 21);
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
        Decoder() :
            _state(Uninitialized)
#ifdef FEATURE_MCCBOR_NESTED
          ,
            _nested(NULLPTR)
#endif
        {}

        Types type() const
        {
            ASSERT_ERROR(true, pos > 0, "Buffer not yet initialized");

            return (Types) (buffer[0] >> 5);
        }

        bool process_iterate(uint8_t value);

        void process(uint8_t value)
        {
            while (!process_iterate(value));
        }

        // TODO: likely this is better suited to a non-inline call
        const uint8_t* process(const uint8_t* value)
        {
            do
            {
                process(*value++);
            }
            while(state() != Done);

            return value;
        }

        enum ParseResult
        {
            OK,
            // length not known at discovery of major type
            // https://tools.ietf.org/html/rfc7049#section-2.2
            //Indefinite,
            // a soft buffer underrun - presented buffer data does not represent entire
            // discovered CBOR type
            Partial,
            // requested type not the one we encountered
            InvalidType
        };


        // Fully untested
        int get_int_experimental(const uint8_t** value, size_t maxlen, ParseResult* result = NULLPTR)
        {
            *value = process(*value);

            switch(type())
            {
                case UnsignedInteger:
                    if(result != NULLPTR) *result = OK;
                    return this->value<int>();

                case NegativeInteger:
                    if(result != NULLPTR) *result = OK;
                    // NOTE: Keep an eye on this, I just hacked this -1 value in here based on cbor.me playground
                    // doublecheck against CBOR spec
                    return -1 - this->value<int>();

                default:
                    if(result != NULLPTR) *result = InvalidType;
                    return 0;
            }
        }


        template <typename TSize>
        TSize get_map_experimental(const uint8_t** value, TSize maxlen, ParseResult* result = NULLPTR)
        {
            *value = process(*value);

            if(type() != Map)
            {
                if(result != NULLPTR) *result = InvalidType;
                return 0;
            }

            return this->value<TSize>();
        }

        // TODO: Probably move this out to non-inline
        // TODO: Make this an internal get_string_array or similar to share array acquisition code
        const_buffer get_string_experimental(const uint8_t** value, size_t maxlen, ParseResult* result = NULLPTR)
        {
            *value = process(*value);
            // TODO: analyze output *value to assist in creating *result

            if(type() != String)
            {
                if(result != NULLPTR) *result = InvalidType;
                return const_buffer((const uint8_t*)NULLPTR, 0);
            }

            // FIX: Need to weed this out of bits_none, bits_8, etc.
            size_t len = this->value<size_t>();

            if(len > maxlen)
            {
                if(result != NULLPTR) *result = Partial;
                len = maxlen;
            }
            else
            {
                if(result != NULLPTR) *result = OK;
            }

            const_buffer chunk(*value, len);

            *value += len;

            return chunk;
        }

        State state() const { return _state; }

#if CBOR_FEATURE_FLOAT
        // might have to do one of these https://beej.us/guide/bgnet/examples/ieee754.c
        float32_t value_float32()
        {
            return -1;
        }
#endif
    };


    // Experimental, mainly for flattening into in-memory hierarchy
    // of CBOR.  Also expected that incoming CBOR will have to be a full flat
    // memory model (ala netbuf or possibly pipeline, but not stream probably)
    class DOM
    {
    public:
        typedef char* string_t;

        class Node {};

        Node& body();

        Node& getElementById(const string_t id);
    };


    // Experimental
    // data_t will be
    // a) a fixed-length array of some variety
    // b) some kind of integer representation (likely similar to the fluid-bitness approach of Decoder)
    // c) an indicator of map, but mainly only the length (or indicator of indefinite) contents will follow later
    //    in subsequent on_element calls
    class IDecoderObserverNew
    {
    public:
        typedef char* string_t;
        typedef void* data_t;

        virtual void on_element(const string_t key, Types type, data_t data) = 0;

        virtual void on_completed() = 0;
    };

    class IDecoderObserver
    {
    public:
        virtual void OnTag(const Decoder& decoder) = 0;

        // when this is fired, the integer/count information is all resolved also
        virtual void OnType(const Decoder& decoder) = 0;

        // fired potentially multipled time for byte array, string, item array
        virtual void OnArray(const Decoder& decoder) = 0;

        // fired potentially multiple times for map
        virtual void OnMap(const Decoder& decoder) = 0;

        virtual void OnFloat(const Decoder& decoder) = 0;

        virtual void OnCompleted() = 0;
    };


    class Encoder : coap::StateHelper<Decoder::State>
    {
        typedef coap::StateHelper<Decoder::State> base_t;

    public:
        Encoder() : base_t(Decoder::Uninitialized) {}

        static uint8_t create_major_type(Types type, uint8_t additional)
        {
            return (type << 5) | additional;
        }

        // FIX: Oops, pretty sure this *ISN'T* how we want to do this...
        void encode(uint8_t value);

        void encode_uint16(uint16_t value)
        {
            // Right, we have to work out pipeline/stream/memory buffer (or whatever that cool
            // name I thought of was...) NETBUF!! yes
        }
    };


    class DecodeToObserver
    {
    public:
        IDecoderObserver* observer;

        void decode(const uint8_t* cbor, size_t len);
    };
};

template <>
inline bool CBOR::Decoder::is_bitness<uint8_t>() const { return is_8bit(); }

template <>
inline bool CBOR::Decoder::is_bitness<uint16_t>() const { return is_16bit(); }

template <>
inline bool CBOR::Decoder::is_bitness<uint32_t>() const { return is_32bit(); }

}

#endif // CBOR_H
