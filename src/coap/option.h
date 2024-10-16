/**
 * References:
 *
 * 1. https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#content-formats
 */
#pragma once

#include "platform.h"

#include <estd/cstdint.h>

namespace embr { namespace coap {

// Base helper class to assist with ALL option related things.  Mainly
// a holder for enums
class Option
{
public:
    enum ExtendedMode
    // TODO: Actually it's gonna be FEATURE_CPP_ENUM_BASE, but they are pretty much the same feature
#ifdef FEATURE_CPP_ENUM_CLASS
        : uint8_t
#endif
    {
        Extended8Bit = 13,
        Extended16Bit = 14,
        Reserved = 15
    };

    enum ContentFormats : uint16_t
    {
        // As per [1]
        ImageGif                = 21,
        ImageJpeg               = 22,
        ImagePng                = 23,

        // RFC 7252 Section 12.3
        TextPlain               = 0,
        ApplicationLinkFormat   = 40,
        ApplicationXml          = 41,
        ApplicationOctetStream  = 42,
        ApplicationExi          = 47,
        ApplicationJson         = 50,

        // As per [1]
        ApplicationPatchJson    = 51,
        ApplicationMergeJson    = 52,

        // RFC 7049
        ApplicationCbor         = 60,

        // As per [1]
        ApplicationCwt              = 61,
        ApplicationMultipartCore    = 62,
        ApplicationCborSeq          = 63,

        ApplicationCoapGroupJson    = 256,      // RFC 7390

        // As per RFC7252 Section 12.3, this
        // "MUST NOT be used in operational deployments".  We use this
        // in a limited capacity to indicate a null content format internally,
        // never over the wire.
        ContentFormatNull           = 0xFFFF
    };

    enum Numbers
    {
        // custom value to initialize to zero for convenience with delta operations
        Zeroed = 0,
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
        Block2 = 23,
        // response payload block-wise
        Block1 = 27,
        Size2 = 28,         // https://datatracker.ietf.org/doc/html/rfc7959#section-4
        ProxyUri = 35,
        ProxyScheme = 39,
        Size1 = 60,         // https://datatracker.ietf.org/doc/html/rfc7959#section-4

        // https://www.rfc-editor.org/rfc/rfc9175.html
        Echo = 252,
        RequestTag = 292
    };


    // ValueStart is always processed, even on option length 0 items
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
        ValueStart, // when we are ready to begin processing value.  This is reached even when no value (aka 0-length) is present
        OptionValue, // processing value portion (this decoder merely skips value, outer processors handle it)
        OptionValueDone, // done with value portion.  Also indicates completion of option, even with zero-length value
        OptionDone, // Not used yet, but indicates option done - I might prefer this to overloading OptionValueDone
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

namespace internal {

typedef Option::Numbers option_number_t;

}

}}
