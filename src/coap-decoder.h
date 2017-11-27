//
// Created by Malachi Burke on 11/25/17.
// decoders take raw bytes as input and create meaningful output from them
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

#include "coap.h"

namespace moducom { namespace coap {


// Not really used
template <size_t buffer_size>
class RawDecoder
{
    uint8_t pos;
    uint8_t buffer[buffer_size];

    inline bool process_iterate(uint8_t value, size_t max_size)
    {
        buffer[pos++] = value;
        return pos == max_size;
    }
};


class HeaderDecoder : public CoAP::Header
{
    uint8_t pos;

public:
    HeaderDecoder() : pos(0) {}

    inline bool process_iterate(uint8_t value)
    {
        bytes[pos++] = value;
        return pos == 4;
    }
};

// processes bytes input to then reveal more easily digestible coap options
// same code that was in CoAP::Parser but dedicated only to option processing
// enough of a divergence I didn't want to gut the old one, thus the copy/paste
// gut the old one once this is proven working
class OptionDecoder
{
public:
    // Need this because all other Option classes I've made are const'd out,
    // but we do need a data entity we can build slowly/iteratively, so that's
    // this --- for now
    // Only reporting number & length since value portion is directly accessible via pipeline
    // coap uint's are a little unusual, but still quite easy to process
    // (https://tools.ietf.org/html/rfc7252#section-3.2)
    struct OptionExperimental
    {
        // delta, not absolute, number.  external party will have to translate this
        // to absolute.  Plot twist: we *add* to this as we go, so if we pass in same
        // OptionExperimental, then that's enough to qualify as an external party
        // consequence: be sure number_delta is initialized at the beginning with 0
        uint16_t number_delta;
        // length of option value portion
        uint16_t length;
    };

private:
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

public:
    uint16_t option_delta() const
    {
        return CoAP::Option_OLD::get_value(raw_delta(), &buffer[1], NULLPTR);
    }

    uint16_t option_length() const
    {
        return CoAP::Option_OLD::get_value(raw_length(), &buffer[1], NULLPTR);
    }

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

    // set state specifically to OptionValueDone
    // useful for when we fast-forward past value portion and want to restart our state machine
    // NOTE: perhaps we'd rather do in-place new instead
    void done() { state(OptionValueDone); }

public:
    State state() const { return _state; }

    bool process_iterate(uint8_t value);

    // only processes until the beginning of value (if present) and depends on caller to read in
    // and advance pipeline past value.  Therefore, sets state machine to OptionValueDone "prematurely"
    bool process_iterate(pipeline::IBufferedPipelineReader& reader, OptionExperimental* built_option);
};

// re-write and decomposition of IResponder
struct IHeaderInput
{
    virtual void on_header(CoAP::Header header) = 0;
};


struct ITokenInput
{
    // get called repeatedly until all portion of token is provided
    // Not called if header reports a token length of 0
    virtual void on_token(const pipeline::PipelineMessage* token_part) = 0;
};

struct IOptionInput
{
    typedef CoAP::OptionExperimental::Numbers number_t;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) = 0;

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(const pipeline::PipelineMessage* option_value_part) = 0;
};


struct IPayloadInput
{
    // will get called repeatedly until payload is completely provided
    // IMPORTANT: if no payload is present, then payload_part is nullptr
    // this call ultimately marks the end of the coap message (unless I missed something
    // for chunked/multipart coap messages... which I may have)
    virtual void on_payload(const pipeline::PipelineMessage* payload_part) = 0;
};

namespace experimental
{

class PipelineReaderDecoderBase
{
protected:
};

// FIX: crap name
template <class TDecoder>
class PipelineReaderDecoder {};

template <>
class PipelineReaderDecoder<OptionDecoder>
{
    OptionDecoder::OptionExperimental built_option;
    OptionDecoder decoder;

public:
    PipelineReaderDecoder<OptionDecoder>()
    {
        built_option.number_delta = 0;
    }

    bool process_iterate(pipeline::IBufferedPipelineReader& reader)
    {
        // FIX: pretty sure OptionDecoder doesn't discover end of options/beginning of payload
        // marker like we'll need it to
        return decoder.process_iterate(reader, &built_option);
    }

    const OptionDecoder::OptionExperimental& option() const { return built_option; }

    uint16_t number_delta() const { return built_option.number_delta; }
    uint16_t length() const { return built_option.length; }
};


template <>
class PipelineReaderDecoder<HeaderDecoder>
{
    HeaderDecoder decoder;

public:
    bool process_iterate(pipeline::IBufferedPipelineReader& reader)
    {
        pipeline::PipelineMessage msg = reader.peek();

        size_t counter = 0;

        while(msg.length--)
        {
            if(decoder.process_iterate(msg[counter++]))
            {
                reader.advance_read(counter);
                return true;
            }
        }

        reader.advance_read(counter);
        return false;
    }

    const CoAP::Header& header() const { return decoder; }
};

/*
inline bool decode_to_input_iterate(PipelineReaderDecoder<OptionDecoder>& decoder, pipeline::IBufferedPipelineReader& reader, IOptionInput& input)
{
    decoder.process_iterate(reader);
}
*/
}

}}

#endif //MC_COAP_TEST_COAP_DECODER_H
