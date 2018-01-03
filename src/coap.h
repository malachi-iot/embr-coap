//
// Created by malachi on 10/20/17.
//

#include "platform.h"
#include "mc/pipeline.h"
#include <stdint.h>
#include <stdlib.h>
#include <new>

#ifndef SRC_COAP_H
#define SRC_COAP_H

// temporary cleanup flag.  get rid of this before merging out of cleanup branch
#define CLEANUP_COAP_CPP

namespace moducom {
namespace coap {

#define COAP_HEADER_FIXED_VER_POS       6
#define COAP_HEADER_FIXED_TYPE_POS      4
#define COAP_HEADER_FIXED_TKL_POS       0

#define COAP_HEADER_FIXED_TYPE_MASK     (3 << COAP_HEADER_FIXED_TYPE_POS)
#define COAP_HEADER_FIXED_TKL_MASK      (15)

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

#define COAP_RESPONSE_CODE(_class, _detail) ((_class << 5) | _detail)

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

class Root
{
public:
    enum State
    {
        Header,
        HeaderDone,
        Token,
        TokenDone,
        OptionsStart,
        Options,
        OptionsDone, // all options are done being processed
        Payload // Note that Payload is *NOT* handled by this class, since its length is defined by transport layer
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
    // https://tools.ietf.org/html/rfc7252#section-3
    class Header
    {
        // temporary public while building code
    public:
        class Code
        {
            friend class Header;

            uint8_t _code;

            // FIX: for internal use only
            Code() {}
        public:

            Code(uint8_t _code) : _code(_code) {}

            enum Classes
            {
                Request = 0,
                Success = 2,
                ClientError = 4,
                ServerError = 5
            };

            // RFC7252 Section 12.1.2
            enum Codes
            {
                Empty =             COAP_RESPONSE_CODE(0, 00),
                Created =           COAP_RESPONSE_CODE(Success, 01),
                Deleted =           COAP_RESPONSE_CODE(Success, 02),
                Valid =             COAP_RESPONSE_CODE(Success, 03),
                Changed =           COAP_RESPONSE_CODE(Success, 04),
                Content =           COAP_RESPONSE_CODE(Success, 05),
                BadRequest =        COAP_RESPONSE_CODE(ClientError, 00),
                Unauthorized =      COAP_RESPONSE_CODE(ClientError, 01),
                BadOption =         COAP_RESPONSE_CODE(ClientError, 02),
                Forbidden =         COAP_RESPONSE_CODE(ClientError, 03),
                NotFound =          COAP_RESPONSE_CODE(ClientError, 04),
                MethodNotAllowed =  COAP_RESPONSE_CODE(ClientError, 05),
                InternalServerError =   COAP_RESPONSE_CODE(ServerError, 00),
                NotImplemented =        COAP_RESPONSE_CODE(ServerError, 01),
                ServiceUnavailable =    COAP_RESPONSE_CODE(ServerError, 03),
                ProxyingNotSupported =  COAP_RESPONSE_CODE(ServerError, 05)
            };


                // topmost 3 bits
            Classes get_class() const { return (Classes)(_code >> 5); }
            // bottommost 5 bits
            uint8_t detail() const { return _code & 0x1F; }

            Codes code() { return (Codes) _code; }
        };

        // RFC 7252 Section 12.1.1
        enum RequestMethodEnum
        {
            Get = 1,
            Post = 2,
            Put = 3,
            Delete = 4
        };

        union
        {
            uint8_t bytes[4];
            // beware raw exposes endian issues, so only use this for bulk assignment/copy
            uint32_t raw;
        };

    protected:

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

        template <uint8_t pos, uint32_t mask_value>
        inline uint8_t mask_fixed(uint8_t byte_pos) const
        {
            return ((bytes[byte_pos] & mask_value) >> pos);
        }

    public:

        uint8_t code() const
        {
#ifdef BROKEN
            return (uint8_t)mask<COAP_HEADER_CODE_POS, COAP_HEADER_CODE_MASK>();
#else
            return bytes[1];
#endif
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
#ifdef BROKEN
            return (TypeEnum) mask<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>();
#else
            return (TypeEnum) mask_fixed<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>(1);
#endif
        }

    protected:
        template <uint8_t pos>
        inline void mask_or(uint8_t byte_pos, uint8_t value)
        {
            bytes[byte_pos] |= value << pos;
        }

    public:

        void type(TypeEnum type)
        {
#ifdef BROKEN
            mask<COAP_HEADER_TYPE_POS, COAP_HEADER_TYPE_MASK>((uint16_t)type);
#else
            bytes[0] &= ~COAP_HEADER_FIXED_TYPE_MASK;
            bytes[0] |= ((uint8_t)type) << COAP_HEADER_FIXED_TYPE_POS;
            mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
#endif
        }

        Code& code_experimental() const
        {
        	void* buffer = (void*)&bytes[1];
        	Code* instance = new (buffer) Code;
            return *instance;
        }

        bool is_request() const
        {
            return (code() >> 5) == 0;
        }

        bool is_response() const { return !is_request(); }

        Code::Codes response_code() const
        {
            ASSERT_WARN(true, is_response(), "Invalid response code detected");

            return Code(code()).code();
        }

        void response_code(Code::Codes value)
        {
            code((Code::Codes)value);
        }

        RequestMethodEnum request_method() const
        {
            return (RequestMethodEnum) code();
        }


        void code(uint8_t code)
        {
#ifdef BROKEN
            mask<COAP_HEADER_CODE_POS, COAP_HEADER_CODE_MASK>(code);
#else
            bytes[1] = code;
#endif
        }


        void message_id(uint16_t mid)
        {
#ifdef BROKEN
            mask<COAP_HEADER_MID_POS, COAP_HEADER_MID_MASK>(mid);
#else
            // slightly clumsy but endianness should be OK
            bytes[2] = mid >> 8;
            bytes[3] = mid & 0xFF;
#endif
        }

        uint16_t message_id() const
        {
#ifdef BROKEN
            uint16_t mid = mask<COAP_HEADER_MID_POS, COAP_HEADER_MID_MASK>();

            return COAP_NTOHS(mid);
#else
            uint16_t mid = bytes[2];
            mid <<= 8;
            mid |= bytes[3];
            return mid;
#endif
        }

        void token_length(uint8_t tkl)
        {
            ASSERT(true, tkl <= 8);

#ifdef BROKEN
            mask<COAP_HEADER_TKL_POS, COAP_HEADER_TKL_MASK>(tkl);
#else
            bytes[0] &= ~COAP_HEADER_FIXED_TKL_MASK;
            bytes[0] |= tkl;
#endif
        }


        uint8_t token_length()
        {
#ifdef BROKEN
            return (uint8_t) mask<COAP_HEADER_TKL_POS, COAP_HEADER_TKL_MASK>();
#else
            return bytes[0] & COAP_HEADER_FIXED_TKL_MASK;
#endif
        }

    //private:
        // FIX: make this private & friended - asking for trouble otherwise
        // with uninitialized data
        Header() {}

    public:

        Header(TypeEnum type)
        {
            raw = 0;
#ifdef BROKEN
            // FIX: 100% endian malfunction, does not work
            raw |= 1 << COAP_HEADER_VER_POS;
            this->type(type);
#else
            mask_or<COAP_HEADER_FIXED_VER_POS>(0, 1);
            mask_or<COAP_HEADER_FIXED_TYPE_POS>(0, type);
#endif
        }
    };
#endif

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

#ifndef CLEANUP_COAP_CPP
    // TODO: Rename this to something more like "IncomingMessageHandler"
    // since "Response" carries particular meaning in CoAP request/response behavior
    // TODO: Rename the OnX functions to on_x
    class IResponderDeprecated
    {
    public:
        //! Responds to a header found in a CoAP message
        //! \param header
        virtual void OnHeader(const Header header) = 0;
        //!
        //! \param message
        //! \param length
        // NOTE: technically we have token length from header...
        virtual void OnToken(const uint8_t message[], size_t length) = 0;
        virtual void OnOption(const OptionExperimentalDeprecated& option) = 0;
        virtual void OnPayload(const uint8_t message[], size_t length) = 0;
        // NOTE: Would like the convenience of OnPayload and OnCompleted combined, but that
        // breaks consistency with the naming.  I suppose I could name it OnPayloadAndOrCompleted
        // but that seems clumsy too
        virtual void OnCompleted() {}
    };


    class IOutgoingMessageHandler
    {

    };

    // this class is deprecated, but not fully functionally replaced yet
    // rework this to an overall "Decoder" class to sit alongside OptionDecoder,
    // HeaderDecoder, etc.  Right now the new experimental::Dispatcher fuses in
    // what will eventually be the Decoder class, which does 99% of what this Parser
    // does, but in a more discrete and organized way
    class ParserDeprecated
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
            Token,
            TokenDone,
            OptionsStart,
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
            OptionValueDone // done with value portion.  Also indicates completion of option, even with zero-length value
        };

    private:
        // remember this is in "network byte order", meaning that
        // message ID will be big endian
        //uint32_t header;

        // Which part of CoAP message we are processing
        State _state;

        union
        {
            // Which part of the option we are processing
            SubState _sub_state;
            // how long token is that we are processing
            uint8_t _token_length;
        };

        void sub_state(SubState sub_state) { _sub_state = sub_state; }
        void state(State state) { _state = state; }

        // small temporary buffer needed for OPTION and HEADER processing
        union
        {
            // By the time we use option size, we've extracted it from buffer and no longer use buffer
            uint16_t option_size;
            // buffer needs only be:
            //  3 for option processing
            //  4 for header processing
            //  8 for token processing
            uint8_t buffer[8];
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

        ParserDeprecated();

        State state() const { return _state; }
        SubState sub_state() const { return _sub_state;  }
        uint8_t* header() { return &buffer[0]; }

        // for use only during token processing
        uint8_t token_length() const { return _token_length; }
        const uint8_t* token() const { return buffer; }

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
    class ParseToIResponderDeprecated
    {
    protected:
        IResponderDeprecated* const responder;
        ParserDeprecated parser;

    public:
        ParseToIResponderDeprecated(IResponderDeprecated* responder) : responder(responder)
        {

        }

        void process(const uint8_t message[], size_t length);
    };


    // FIX: Needs an even better name than its parent
    class ParseIterateToIResponderDeprecated : protected ParseToIResponderDeprecated
    {
        // Experimental Anticipated maximum size of incoming option
        pipeline::layer3::MemoryChunk<64> chunk;

        struct option_state_t
        {
            uint16_t number;
            uint16_t length;
            const uint8_t* value;
        };

        union
        {
            option_state_t  option;
        } state;


    public:
        ParseIterateToIResponderDeprecated(IResponderDeprecated* responder) : ParseToIResponderDeprecated(responder) {}

        bool process_iterate(const pipeline::MemoryChunk& incoming);
        void process_done()
        {
            responder->OnCompleted();
        }

        void reset_experimental(IResponderDeprecated* responder)
        {
            // TODO: see if there's a convenient way to do this (in place delete/new...)
            //this->responder = responder;
        }

    };

    class OptionParser
    {
    public:
        void process(uint8_t value);
    };
#endif

};

namespace experimental {


typedef Option::Numbers option_number_t;
//typedef CoAP::OptionExperimentalDeprecated::ValueFormats option_value_format_t;
typedef Option::ContentFormats option_content_format_t;

typedef CoAP::Header::TypeEnum header_type_t;
typedef CoAP::Header::Code::Codes header_response_code_t;
typedef CoAP::Header::RequestMethodEnum header_request_code_t;

typedef Option::ExtendedMode extended_mode_t;
typedef Option _extended_mode_t;

}

}
}


#endif //SRC_COAP_H
