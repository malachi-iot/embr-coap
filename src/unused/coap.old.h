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