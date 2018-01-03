//
// Created by Malachi Burke on 11/25/17.
// decoders take raw bytes as input and create meaningful output from them
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

#include "coap.h"

namespace moducom { namespace coap {


template <class TState>
class StateHelper
{
    TState _state;

protected:
    void state(TState _state) { this->_state = _state; }

    StateHelper(TState _state) { state(_state); }

public:
    StateHelper() {}

    TState state() const { return _state; }
};




// TODO: Move this into a better namespace
/*!
 * Simply counts down number of bytes as a "decode" operation
 */
template <typename TCounter = size_t>
class CounterDecoder
{
    TCounter pos;

protected:
    TCounter position() const { return pos; }

public:
    CounterDecoder() : pos(0) {}

    // returns true when we achieve our desired count
    inline bool process_iterate(TCounter max_size)
    {
        return ++pos >= max_size;
    }

    void reset() { pos = 0; }
};


template <size_t buffer_size, typename TCounter=uint8_t>
class RawDecoder : public CounterDecoder<TCounter>
{
    uint8_t buffer[buffer_size];

public:
    typedef CounterDecoder<TCounter> base_t;

    uint8_t* data() { return buffer; }

    // returns true when all bytes finally accounted for
    inline bool process_iterate(uint8_t value, TCounter max_size)
    {
        buffer[base_t::position()] = value;
        return base_t::process_iterate(max_size);
    }
};

typedef RawDecoder<8> TokenDecoder;


class HeaderDecoder : 
    public CoAP::Header,
    public CounterDecoder<uint8_t>
{
    typedef CounterDecoder<uint8_t> base_t;

public:
    // returns true when complete
    inline bool process_iterate(uint8_t value)
    {
        bytes[base_t::position()] = value;
        return base_t::process_iterate(4);
    }
};

class OptionDecoderBase
{
public:
    // copy/pasted from "ParserDeprecated"
    enum State
    {
        //OptionStart, // header portion for pre processing
                OptionSize, // header portion.  This serves also as OptionSizeBegin, since OptionSize is only one byte ever
        OptionSizeDone, // done processing size-header portion
        OptionDelta, // processing delta portion (after header, if applicable)
        OptionDeltaDone, // done with delta portion, ready to move on to Length
        OptionLength, // processing length portion (after header, if applicable)
        OptionLengthDone, // done with length portion
        OptionDeltaAndLengthDone, // done with both length and size, happens only when no extended values are used
        OptionValue, // processing value portion (this decoder merely skips value, outer processors handle it)
        OptionValueDone, // done with value portion.  Also indicates completion of option, even with zero-length value
        Payload // payload marker found
    };

};

// processes bytes input to then reveal more easily digestible coap options
// same code that was in CoAP::ParserDeprecated but dedicated only to option processing
// enough of a divergence I didn't want to gut the old one, thus the copy/paste
// gut the old one once this is proven working
class OptionDecoder :
        public OptionDecoderBase,
        public StateHelper<OptionDecoderBase::State>
{
public:
    typedef CoAP::OptionExperimentalDeprecated _number_t;

    // TODO: Move this to OptionDecoder
    enum ValueFormats
    {
        Unknown = -1,
        Empty,
        Opaque,
        UInt,
        String
    };



    // Need this because all other Option classes I've made are const'd out,
    // but we do need a data entity we can build slowly/iteratively, so that's
    // this --- for now
    // Only reporting number & length since value portion is directly accessible via pipeline
    // coap uint's are a little unusual, but still quite easy to process
    // (https://tools.ietf.org/html/rfc7252#section-3.2)
    // FIX: Find a good name.  "Option" or "OptionRaw" not quite right because this excludes
    // the value portion.  Perhaps "OptionPrefix"
    struct OptionExperimental
    {
        // delta, not absolute, number.  external party will have to translate this
        // to absolute.  Plot twist: we *add* to this as we go, so if we pass in same
        // OptionExperimentalDeprecated, then that's enough to qualify as an external party
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
        // we use this primarily as a countdown to skip past value
        uint16_t option_size;
        // buffer needs only be:
        //  3 for option processing
        //  4 for header processing
        //  8 for token processing
        uint8_t buffer[4];
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


    uint16_t option_delta() const
    {
        return get_value(raw_delta(), &buffer[1], NULLPTR);
    }

    uint16_t option_length() const
    {
        return get_value(raw_length(), &buffer[1], NULLPTR);
    }

private:
    // set state specifically to OptionValueDone
    // useful for when we fast-forward past value portion and want to restart our state machine
    // NOTE: perhaps we'd rather do in-place new instead
    void done() { state(OptionValueDone); }

public:
    OptionDecoder() : StateHelper(OptionSize) {}

    static ValueFormats get_format(experimental::option_number_t number)
    {
        switch (number)
        {
            case _number_t::IfMatch:         return Opaque;
            case _number_t::UriHost:         return String;
            case _number_t::ETag:            return Opaque;
            case _number_t::IfNoneMatch:     return Empty;
            case _number_t::UriPort:         return UInt;
            case _number_t::LocationPath:    return String;
            case _number_t::UriPath:         return String;
            case _number_t::ContentFormat:   return UInt;

            case _number_t::Block1:
            case _number_t::Block2:
                return UInt;

            default:
                return Unknown;
        }
    }

    bool process_iterate(uint8_t value);

    // only processes until the beginning of value (if present) and depends on caller to read in
    // and advance pipeline past value.  Therefore, sets state machine to OptionValueDone "prematurely"
    bool process_iterate(pipeline::IBufferedPipelineReader& reader, OptionExperimental* built_option);

    // NOTE: this seems like the preferred method, here.  Could be problematic for option value
    // though (as would the preceding two versions of this function).  However, since caller
    // is one providing data, it may be reasonable to expect caller to assemble the value themselves
    // and merely allow process_iterate to detect where the boundaries are
    //
    // returns number of raw bytes processed.  May return before processing entire option in order
    // to give caller chance to prepare for option value contents.  Specifically, we stop on
    // OptionLengthDone, OptionDeltaAndLengthDone and OptionValueDone boundaries.  Eventually
    // we will also stop on OptionValue occasionally if option-value size is larger than the
    // input chunk
    size_t process_iterate(const pipeline::MemoryChunk& input, OptionExperimental* built_option);
};


class DecoderBase
{
public:
    // Copy/paste from old "ParserDeprecated" class
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

    typedef State state_t;
};


class Decoder :
    public DecoderBase,
    public StateHelper<DecoderBase::State>
{
protected:
    // TODO: Union-ize this  Not doing so now because of C++03 trickiness
    HeaderDecoder headerDecoder;
    OptionDecoder optionDecoder;
    OptionDecoder::OptionExperimental optionHolder;
    TokenDecoder tokenDecoder;


    inline void init_header_decoder() { new (&headerDecoder) HeaderDecoder; }

    // NOTE: Initial reset redundant since class initializes with 0, though this
    // may well change when we union-ize the decoders.  Likely though instead we'll
    // use an in-place new
    inline void init_token_decoder() { tokenDecoder.reset(); }

    // NOTE: reset might be more useful if we plan on not auto-resetting
    // option decoder from within its own state machine
    inline void init_option_decoder()
    {
        new (&optionDecoder) OptionDecoder;
        //optionDecoder.reset();
    }

public:
    Decoder() : StateHelper(DecoderBase::Uninitialized) {}

    bool process_iterate();
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
