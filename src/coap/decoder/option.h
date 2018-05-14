#pragma once

namespace moducom { namespace coap {

// processes bytes input to then reveal more easily digestible coap options
// same code that was in CoAP::ParserDeprecated but dedicated only to option processing
// enough of a divergence I didn't want to gut the old one, thus the copy/paste
// gut the old one once this is proven working
class OptionDecoder :
        public Option,
        public StateHelper<Option::State>
{
public:
    typedef Option _number_t;
    typedef experimental::option_number_t number_t;

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
        uint16_t value_length;
        // buffer needs only be:
        //  3 for option processing
        //  4 for header processing
        //  8 for token processing
        uint8_t buffer[4];
    };
    uint8_t pos;

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
    OptionDecoder() : StateHelper(FirstByte) {}

    /*
    static uint16_t get_max_length(number_t number)
    {
        switch(number)
        {
            case IfMatch:   return 8;
        }
    }*/

    // Do this instead with specialized template trait, then we can handle min, max and type

    static ValueFormats get_format(number_t number)
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

#ifdef UNUSED
    // only processes until the beginning of value (if present) and depends on caller to read in
    // and advance pipeline past value.  Therefore, sets state machine to OptionValueDone "prematurely"
    // FIX: Phase out this one
    bool process_iterate(pipeline::IBufferedPipelineReader& reader, OptionExperimental* built_option);
#endif

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
    size_t process_iterate(const pipeline::MemoryChunk::readonly_t& input, OptionExperimental* built_option);
};

}}
