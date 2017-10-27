//
// Created by malachi on 10/20/17.
//

#include "platform.h"
#include <stdint.h>
#include <stdlib.h>

#ifndef SRC_COAP_H
#define SRC_COAP_H

namespace moducom {
namespace coap {

#define COAP_HEADER_VER_POS     30
#define COAP_HEADER_TYPE_POS    28
#define COAP_HEADER_TKL_POS     24
#define COAP_HEADER_CODE_POS    16
#define COAP_HEADER_MID_POS     0

#define COAP_HEADER_VER_MASK    (3 << COAP_HEADER_VER_POS)
#define COAP_HEADER_TYPE_MASK   (3 << COAP_HEADER_TYPE_POS)
#define COAP_HEADER_TKL_MASK    (15 << COAP_HEADER_TKL_POS)
#define COAP_HEADER_CODE_MASK   (255 << COAP_HEADER_CODE_POS)
#define COAP_HEADER_MID_MASK    (0xFFFF)


#define COAP_OPTION_DELTA_POS   4
#define COAP_OPTION_DELTA_MASK  15
#define COAP_OPTION_LENGTH_POS  0
#define COAP_OPTION_LENGTH_MASK 15

#define COAP_EXTENDED8_BIT_MAX  (255 - 13)
#define COAP_EXTENDED16_BIT_MAX (0xFFFF - 269)

// potentially http://pubs.opengroup.org/onlinepubs/7908799/xns/htonl.html are of interest here
// also endianness macros here if you are in GNU:
// https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html

// given a 16-bit value, swap bytes
#define SWAP_16(value) (((value & 0xFF) << 8) | (value >> 8))
// convert a 16-bit little endian number to big endian
#define LE_TO_BE_16(value)

// See RFC 7252: https://tools.ietf.org/html/rfc7252
class CoAP
{
public:
    // Unfortunately, bitfields are not gaurunteed to be ordered in a particular order
    // https://stackoverflow.com/questions/1490092/c-c-force-bit-field-order-and-alignment
#ifdef BITFIELD_VERSION
    // https://tools.ietf.org/html/rfc7252#section-3
    class Header
    {
        //protected:
    public:
        // We do need an "uninitialized" version of this (for unions/read scenarios)
        // But generally it's hidden
        //Header() {}

    public:
        enum TypeEnum
        {
            Confirmable = 0,
            NonConfirmable = 1,
            Acknowledgement = 2,
            Reset = 3
        };

        union
        {
            struct PACKED
            {
                uint32_t version : 2;
                TypeEnum type : 2;
                uint32_t token_length : 4;
                uint32_t code : 8;
                uint32_t message_id : 16;
            } cooked;

            uint32_t raw;
        };

        Header(TypeEnum type)
            //type(type)
        {
            raw = 0;
            cooked.version = 1;
            //cooked.type = type;
        }
    };
#else
    // https://tools.ietf.org/html/rfc7252#section-3
    class Header
    {
        // temporary public while building code
    public:
        union
        {
            uint8_t bytes[4];
            uint32_t raw;
        };

        template <uint8_t pos, uint32_t mask_value>
        inline void mask(uint16_t value)
        {
            raw = (raw & ~mask_value) | (value << pos);
        }

        template <uint8_t pos, uint32_t mask_value>
        inline uint16_t mask() const
        {
            return ((raw & mask_value) >> pos);
        }


    public:
        enum TypeEnum
        {
            Confirmable = 0,
            NonConfirmable = 1,
            Acknowledgement = 2,
            Reset = 3
        };

        TypeEnum type() const 
        {
            return (TypeEnum) mask<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>();
        }

        void type(TypeEnum type)
        {
            mask<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>((uint16_t)type);
        }

        uint8_t code() const
        {
            return (uint8_t)mask<COAP_HEADER_CODE_POS, COAP_HEADER_CODE_MASK>();
        }


        void code(uint8_t code)
        {
            mask<COAP_HEADER_CODE_POS, COAP_HEADER_CODE_MASK>(code);
        }


        void message_id(uint16_t mid)
        {
            mask<COAP_HEADER_MID_POS, COAP_HEADER_MID_MASK>(mid);
        }

        uint16_t message_id() const
        {
            uint16_t mid = mask<COAP_HEADER_MID_POS, COAP_HEADER_MID_MASK>();

            return COAP_NTOHS(mid);
        }

        void token_length(uint8_t tkl)
        {
            ASSERT(true, tkl <= 8);

            mask<COAP_HEADER_TKL_POS, COAP_HEADER_TKL_MASK>(tkl);
        }


        uint8_t token_length()
        {
            return (uint8_t) mask<COAP_HEADER_TKL_POS, COAP_HEADER_TKL_MASK>();
        }

        Header() {}


        Header(TypeEnum type)
        {
            raw = 0;
            raw |= 1 << COAP_HEADER_VER_POS;
            this->type(type);
        }
    };
#endif

    class Option_OLD
    {
        uint8_t option_size;
        uint8_t bytes[];

        enum ExtendedMode
#ifdef __CPP11__
                : uint8_t
#endif
        {
            Extended8Bit = 13,
            Extended16Bit = 14,
            Reserved = 15
        };

    public:
        static uint16_t get_value(uint8_t nonextended, const uint8_t* extended, uint8_t* index_bump)
        {
            if (nonextended < Extended8Bit)
            {
                // Just use literal value, not extended
                return nonextended;
            }
            else if (nonextended == Extended8Bit)
            {
                //(*index_bump)++;
                return 13 + *extended;
                //return  *extended;
            }
            else if (nonextended == Extended16Bit)
            {
                // FIX: BEWARE of endian issue!!
                //uint16_t _extended = *((uint16_t*)extended);
                //uint16_t _extended = COAP_UINT16_FROM_NBYTES(extended[0], extended[1]);

                // Always coming in from network byte order (big endian)
                uint16_t _extended = extended[0];
                
                _extended <<= 8;
                _extended |= extended[1];

                //(*index_bump)+=2;

                //return 269 + _extended;
                return 269 + _extended;
            }
            else // RESERVED
            {
                ASSERT_ERROR(Reserved, nonextended, "Code broken - incorrectly assess value as RESERVED");
                ASSERT_ERROR(0, 15, "Invalid length/delta value");
                // TODO: register as an error
            }

            return 0;
        }

    public:



    };

    class Parser;

    // FIX: Need better name
    /// Represents higher level fully built out option for processing at an application level
    class OptionExperimental
    {
    public:
        enum ValueFormats
        {
            Unknown = -1,
            Empty,
            Opaque,
            UInt,
            String
        };

        enum Numbers
        {
            /// format: opaque
            IfMatch = 1,
            // format: string
            UriHost = 3,
            // format: opaque
            ETag = 4,
            IfNoneMatch = 5,
            UriPort = 7,
            LocationPath = 8,
            UriPath = 11,
            ContentFormat = 12

        };

        // Option Number as defined by RFC7252
        const uint16_t number;
        const uint16_t length;

        Numbers get_number() const { return (Numbers)number; }

        ValueFormats get_format() const 
        {
            typedef ValueFormats vf_t;
            typedef Numbers number_t;

            switch (get_number())
            {
                case number_t::IfMatch:         return vf_t::Opaque;
                case number_t::UriHost:         return vf_t::String;
                case number_t::ETag:            return vf_t::Opaque;
                case number_t::IfNoneMatch:     return vf_t::Empty;
                case number_t::UriPort:         return vf_t::UInt;
                case number_t::LocationPath:    return vf_t::String;
                case number_t::UriPath:         return vf_t::String;
                case number_t::ContentFormat:   return vf_t::UInt;
                default:
                    return vf_t::Unknown;
            }
        }

        union
        {
            const uint8_t *value;
            const uint16_t value_uint;
        };

        OptionExperimental(uint16_t number, uint16_t length, const uint8_t* value) :
            number(number),
            length(length),
            value(value)
        {}

        OptionExperimental(uint16_t number, uint16_t length, const uint16_t value_uint) :
                number(number),
                length(length),
                value_uint(value_uint)
        {}
    };


    class IResponder
    {
    public:
        //! Responds to a header found in a CoAP message
        //! \param header
        virtual void OnHeader(const Header header) = 0;
        virtual void OnOption(const OptionExperimental& option) = 0;
        virtual void OnPayload(const uint8_t message[], size_t length) = 0;
    };

    class Parser
    {
        // FIX: temporarily making this public
    public:
        enum ExtendedMode
#ifdef __CPP11__
            : uint8_t
#endif
        {
            Extended8Bit = 13,
            Extended16Bit = 14,
            Reserved = 15
        };


    public:
        enum State
        {
            Header,
            HeaderDone,
            Options,
            OptionsDone, // all options are done being processed
            Payload // Note that Payload is *NOT* handled by this class, since its length is defined by transport layer
        };

        enum SubState
        {
            OptionSize, // header portion.  This serves also as OptionSizeBegin, since OptionSize is only one byte ever
            OptionSizeDone, // done processing size-header portion
            OptionDelta, // processing delta portion (after header, if applicable)
            OptionDeltaDone, // done with delta portion, ready to move on to Length
            OptionLength, // processing length portion (after header, if applicable)
            OptionLengthDone, // done with length portion
            OptionDeltaAndLengthDone, // done with both length and size, happens only when no extended values are used
            OptionValue, // processing value portion (this decoder merely skips value, outer processors handle it)
            OptionValueDone // done with value portion
        };

    private:
        // remember this is in "network byte order", meaning that
        // message ID will be big endian
        //uint32_t header;

        // Which part of CoAP message we are processing
        State _state;

        // Which part of the option we are processing
        SubState _sub_state;

        void sub_state(SubState sub_state) { _sub_state = sub_state; }
        void state(State state) { _state = state; }

        // small temporary buffer needed for OPTION and HEADER processing
        union
        {
            // By the time we use option size, we've extracted it from buffer and no longer use buffer
            uint16_t option_size;
            uint8_t buffer[4];
        };
        // position in buffer we are presently at
        uint8_t pos;

        // TODO: optimize out and calculate in-place based purely on buffer[] then
        // emit via a pointer
        // when processing options, what is the value of the Delta/Length

        bool processOptionSize(uint8_t size_root);

        bool processOption(uint8_t value);

        /* Don't do any of this callback stuff because remember our lesson with workflows:
         * state machines are a great standalone underpinning of event sources
        typedef void (*low_level_callback_t)(void *context);
        typedef void (*callback_t)(void* context);

        low_level_callback_t low_level_callback;

        void * const callback_context;

        // announce an option has been found
        void callback_option();

        // announce a header has been found
        void callback_header(); */

        uint8_t raw_delta() const { return buffer[0] >> 4; }
        uint8_t raw_length() const { return buffer[0] & 0x0F; }

        const uint8_t* get_buffer() const { return (&buffer[0]); }

    public:
        // returns true when the character is *actually* processed
        bool process_iterate(uint8_t value);

        void process(uint8_t value)
        {
            while (!process_iterate(value));
        }

        Parser();

        State state() const { return _state; }
        SubState sub_state() const { return _sub_state;  }
        uint8_t* get_header() { return &buffer[0]; }

        uint16_t option_delta() const
        {
            return Option_OLD::get_value(raw_delta(), &buffer[1], NULLPTR);
        }

        uint16_t option_length() const 
        { 
            return Option_OLD::get_value(raw_length(), &buffer[1], NULLPTR);
        }
    };

    // FIX: Needs much better name
    /// This class is an adapter between low-level decoder and high level IResponder
    class ParseToIResponder
    {
        IResponder* const responder;
        Parser parser;

    public:
        ParseToIResponder(IResponder* responder) : responder(responder)
        {

        }

        void process(uint8_t message[], size_t length);
    };

    class OptionParser
    {
    public:
        void process(uint8_t value);
    };


};

}
}


#endif //SRC_COAP_H
