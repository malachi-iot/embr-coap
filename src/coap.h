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

    class Option
    {
        uint8_t option_size;
        uint8_t bytes[];

        enum ExtendedMode : uint8_t
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



    };

    class Parser
    {
        enum State
        {
            Header = 0,
            Options = 1,
            Payload = 2
        };

        enum ExtendedMode : uint8_t
        {
            Extended8Bit = 13,
            Extended16Bit = 14,
            Reserved = 15
        };


        enum SubState
        {
            OptionSize, // header portion
            OptionDelta, // delta portion (after header, if applicable)
            OptionLength // length portion (after header, if applicable)
        };

        // Which part of CoAP message we are processing
        State state;

        // Which part of the option we are processing
        SubState sub_state;

        // small temporary buffer needed for OPTION processing
        uint8_t buffer[4];
        // position in buffer we are presently at
        uint8_t pos;

        // when processing options, what is the size of the next upcoming option
        uint16_t option_size;

        bool processOptionSize(uint8_t size_root, uint8_t value, uint8_t local_position);

        bool processOptionValue(uint8_t value);

        // returns false when processing is complete, true otherwise
        bool processOption(uint8_t value);

    public:
        void process(uint8_t value);

        Parser();
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
