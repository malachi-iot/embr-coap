//
// Created by malachi on 10/20/17.
//

#include "platform.h"
#include <stdint.h>

#ifndef SRC_COAP_H
#define SRC_COAP_H

namespace moducom {
namespace coap {

#define COAP_HEADER_VER_POS     29
#define COAP_HEADER_TYPE_POS    27
#define COAP_HEADER_TKL_POS     23
#define COAP_HEADER_CODE_POS    15
#define COAP_HEADER_MID_POS     0

#define COAP_HEADER_VER_MASK    (3 << COAP_HEADER_VER_POS)
#define COAP_HEADER_TYPE_MASK   (3 << COAP_HEADER_TYPE_POS)
#define COAP_HEADER_TKL_MASK    (15 << COAP_HEADER_TKL_POS)
#define COAP_HEADER_CODE_MASK   (255 << COAP_HEADER_CODE_POS)
#define COAP_HEADER_MID_MASK    (0xFFFF)

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

        template <uint8_t pos, uint32_t mask>
        inline void mask_setter(uint16_t value)
        {
            raw = (raw & ~mask) | (value << pos);
            //raw &= ~mask;
            //raw |= value << pos;
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
            return (TypeEnum) ((raw & COAP_HEADER_TYPE_MASK) >> COAP_HEADER_TYPE_POS);
        }

        void type(TypeEnum type)
        {
            /*
            raw &= ~COAP_HEADER_TYPE_MASK;
            raw |= (int)type << COAP_HEADER_TYPE_POS;
            */
            mask_setter<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>((uint16_t)type);
        }

        Header(TypeEnum type)
        {
            raw = 0;
            raw |= 1 << (COAP_HEADER_VER_POS + 1);
            this->type(type);
        }
    };
#endif
};

}
}


#endif //SRC_COAP_H
