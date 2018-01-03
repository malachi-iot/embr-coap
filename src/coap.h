//
// Created by malachi on 10/20/17.
//

#include "platform.h"
#include "mc/pipeline.h"
#include <stdint.h>
#include <stdlib.h>
#include <new>

#include "coap-header.h"

#ifndef SRC_COAP_H
#define SRC_COAP_H

// temporary cleanup flag.  get rid of this before merging out of cleanup branch
#define CLEANUP_COAP_CPP

namespace moducom {
namespace coap {

#define COAP_OPTION_DELTA_POS   4
#define COAP_OPTION_DELTA_MASK  15
#define COAP_OPTION_LENGTH_POS  0
#define COAP_OPTION_LENGTH_MASK 15

#define COAP_EXTENDED8_BIT_MAX  (255 - 13)
#define COAP_EXTENDED16_BIT_MAX (0xFFFF - 269)

#define COAP_FEATURE_SUBSCRIPTIONS

// Base helper class to assist with ALL option related things.  Mainly
// a holder for enums
class Option
{
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

    // TODO: Move this to a better home
    enum ContentFormats
    {
        // RFC 7252 Section 12.3
        TextPlain               = 0,
        ApplicationLinkFormat   = 40,
        ApplicationXml          = 41,
        ApplicationOctetStream  = 42,
        ApplicationExi          = 43,
        ApplicationJson         = 44,

        // RFC 7049
        ApplicationCbor         = 60
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
        // https://tools.ietf.org/html/rfc7641#section-2
        // format: uint 0-3 bytes
        Observe = 6,
        UriPort = 7,
        LocationPath = 8,
        UriPath = 11,
        ContentFormat = 12,
        MaxAge = 14,
        UriQuery = 15,
        Accept = 17,
        LocationQuery = 20,
        // https://tools.ietf.org/html/rfc7959#section-2.1
        // request payload block-wise
        Block1 = 23,
        // response payload block-wise
        Block2 = 27,
        ProxyUri = 35,
        ProxyScheme = 39,
        Size1 = 60
    };


    enum State
    {
        //OptionStart, // header portion for pre processing
        FirstByte, // first-byte portion.  This serves also as OptionSizeBegin, since FirstByte is only one byte ever
        FirstByteDone, // done processing first-byte-header portion
        OptionDelta, // processing delta portion (after first-byte, if applicable)
        OptionDeltaDone, // done with delta portion, ready to move on to Length
        OptionLength, // processing length portion (after header, if applicable)
        OptionLengthDone, // done with length portion
        OptionDeltaAndLengthDone, // done with both length and size, happens only when no extended values are used
        ValueStart, // when we are ready to begin processing value.  This is reached even when no value is present
        OptionValue, // processing value portion (this decoder merely skips value, outer processors handle it)
        OptionValueDone, // done with value portion.  Also indicates completion of option, even with zero-length value
        Payload // payload marker found
    };


    enum ValueFormats
    {
        Unknown = -1,
        Empty,
        Opaque,
        UInt,
        String
    };
};

namespace experimental {

// FIX: Need a better name.  Root class of helpers for message-level
// Encoder/Decoder operations
class Root
{
public:
    enum State
    {
        Uninitialized,
        Header,
        HeaderDone,
        Token,
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload,
        PayloadDone,
        // Denotes completion of entire CoAP message
        Done,
    };
};

typedef Root::State root_state_t;
typedef Root _root_state_t;

}

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
#endif


#ifndef CLEANUP_COAP_CPP
    class Option_OLD : Option
    {
        uint8_t option_size;
        uint8_t bytes[];

    public:
        // DEPRECATED - use the one in OptionDecoder instead
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

    class ParserDeprecated;

    /// Represents higher level fully built out option for processing at an application level
    class OptionExperimentalDeprecated
    {
    public:
        typedef Option::Numbers Numbers;



        // Option Number as defined by RFC7252
        const uint16_t number;
        const uint16_t length;

        Numbers get_number() const { return (Numbers)number; }

        typedef Numbers number_t;

        union
        {
            const uint8_t *value;
            // FIX: Looks like we can have uints from 0-32 bits
            // https://tools.ietf.org/html/rfc7252#section-3.2
            const uint16_t value_uint;
        };

        OptionExperimentalDeprecated(uint16_t number, uint16_t length, const uint8_t* value) :
            number(number),
            length(length),
            value_uint(0)
        {
        	this->value = value;
        }

        OptionExperimentalDeprecated(uint16_t number, uint16_t length, const uint16_t value_uint) :
                number(number),
                length(length),
                value_uint(value_uint)
        {}

        // FIX: beware, we have a duplicate of these helpers in coap-blockwise::experimental
        uint8_t get_uint8_t() const
        {
            ASSERT_ERROR(1, length, "Incorrect length");
            return *value;
        }

        uint32_t get_uint16_t() const
        {
            ASSERT_ERROR(2, length, "Incorrect length");

            uint32_t v = *value;

            v <<= 8;
            v |= value[1];

            return v;
        }

        uint32_t get_uint24_t() const
        {
            ASSERT_ERROR(3, length, "Incorrect length");

            uint32_t v = *value;

            v <<= 8;
            v |= value[1];
            v <<= 8;
            v |= value[2];

            return v;
        }

        // Have yet to see a CoAP UINT option value larger than 32 bits
        uint32_t get_uint() const
        {
            ASSERT_ERROR(true, length <= 4, "Option length too large");

            uint32_t v = *value;

            for(int i = 0; i < length; i++)
            {
                v <<= 8;
                v |= value[i];
            }

            return v;
        }
    };
#endif
};

namespace experimental {


typedef Option::Numbers option_number_t;
//typedef CoAP::OptionExperimentalDeprecated::ValueFormats option_value_format_t;
typedef Option::ContentFormats option_content_format_t;

typedef Header::TypeEnum header_type_t;
typedef Header::Code::Codes header_response_code_t;
typedef Header::RequestMethodEnum header_request_code_t;

typedef Option::ExtendedMode extended_mode_t;
typedef Option _extended_mode_t;

}

}
}


#endif //SRC_COAP_H
