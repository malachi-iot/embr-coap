//
// Created by Malachi Burke on 11/25/17.
// decoders take raw bytes as input and create meaningful output from them
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

#include "coap.h"

namespace moducom { namespace coap {

// processes bytes input to then reveal more easily digestible coap options
// same code that was in CoAP::Parser but dedicated only to option processing
// enough of a divergence I didn't want to gut the old one, thus the copy/paste
// gut the old one once this is proven working
class OptionDecoder
{
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
    uint8_t pos;

    enum ExtendedMode
#ifdef __CPP11__
        : uint8_t
#endif
    {
        Extended8Bit = 13,
        Extended16Bit = 14,
        Reserved = 15
    };


    bool processOptionSize(uint8_t size_root);

    uint8_t raw_delta() const { return buffer[0] >> 4; }
    uint8_t raw_length() const { return buffer[0] & 0x0F; }

    uint16_t option_length() const
    {
        return CoAP::Option_OLD::get_value(raw_length(), &buffer[1], NULLPTR);
    }

public:
    enum State
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
    State _state;

    void state(State s) { _state = s; }

public:
    State state() const { return _state; }

    bool process_iterate(uint8_t value);
};

}}

#endif //MC_COAP_TEST_COAP_DECODER_H
