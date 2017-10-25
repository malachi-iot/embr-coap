//
// Created by malachi on 10/20/17.
//

#include "platform.h"
#include <stdint.h>

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
        uint32_t raw;

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

        uint16_t messaged_id() const
        {
            return mask<COAP_HEADER_MID_POS, COAP_HEADER_MID_MASK>();
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


        Header(TypeEnum type)
        {
            raw = 0;
            raw |= 1 << COAP_HEADER_VER_POS;
            this->type(type);
        }
    };
#endif

/*
    class Option
    {
        uint8_t option_size;
        uint8_t bytes[];

        enum ExtendedMode
                : uint8_t
        {
            Extended8Bit = 13,
            Extended16Bit = 14,
            Reserved = 15
        };

        static uint16_t get_value(uint8_t nonextended, uint8_t* extended, uint8_t* index_bump)
        {
            if (nonextended < Extended8Bit)
            {
                // Just use literal value, not extended
                return nonextended;
            }
            else if (nonextended == Extended8Bit)
            {
                (*index_bump)++;
                return 13 + *extended;
            }
            else if (nonextended == Extended16Bit)
            {
                // FIX: BEWARE of endian issue!!
                uint16_t _extended = *((uint16_t*)extended);

                (*index_bump)+=2;

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



    }; */

    class Parser;

    // FIX: Need better name
    /// Represents higher level fully built out option for processing at an application level
    class OptionExperimental
    {
    public:
        // Option Number as defined by RFC7252
        const uint16_t number;
        const uint16_t length;
        const uint8_t* value;

        OptionExperimental(uint16_t number, uint16_t length, const uint8_t* value) :
            number(number),
            length(length),
            value(value)
        {}
    };


    class OptionString
    {
    public:

    };

    class IResponder
    {
    public:
        virtual void OnHeader(const Header header) = 0;
        virtual void OnOption(const OptionExperimental& option) = 0;
        virtual void OnPayload(const uint8_t message[], size_t length) = 0;
    };

    class Parser
    {
        enum ExtendedMode :
            uint8_t
        {
            Extended8Bit = 13,
            Extended16Bit = 14,
            Reserved = 15
        };


    public:
        enum State
        {
            Header = 0,
            Options = 1,
            Payload = 2 // Note that Payload is *NOT* handled by this class, since its length is defined by transport layer
        };

        enum SubState
        {
            OptionSize, // header portion
            OptionDelta, // processing delta portion (after header, if applicable)
            OptionDeltaDone, // done with delta portion
            OptionLength, // processing length portion (after header, if applicable)
            OptionLengthDone, // done with length portion
            OptionValue, // processing value portion (this decoder merely skips value, outer processors handle it)
            OptionValueDone // done with value portion
        };

    private:
        // remember this is in "network byte order", meaning that
        // message ID will be big endian
        uint32_t header;

        // Which part of CoAP message we are processing
        State state;

        // Which part of the option we are processing
        SubState sub_state;

        // small temporary buffer needed for OPTION and HEADER processing
        uint8_t buffer[4];
        // position in buffer we are presently at
        uint8_t pos;
        // size of CoAP HEADER+OPTIONS
        uint16_t nonPayLoadSize;

        // TODO: optimize out and calculate in-place based purely on buffer[] then
        // emit via a pointer
        // when processing options, what is the value of the Delta/Length
        uint16_t option_size;

        bool processOptionSize(uint8_t size_root);

        // returns false when processing is complete, true otherwise
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

    public:
        void process(uint8_t value);

        Parser();

        State get_state() const { return state; }
        SubState get_sub_state() const { return sub_state;  }
        uint16_t get_option_size() const { return option_size; }
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
