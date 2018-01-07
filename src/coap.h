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
    // for IPipelineWriter, mainly
    typedef uint8_t boundary_t;

    
    static CONSTEXPR boundary_t boundary_segment = 3;
    static CONSTEXPR boundary_t boundary_message = 4;

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
