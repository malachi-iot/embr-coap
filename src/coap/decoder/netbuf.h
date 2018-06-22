#pragma once

#include "coap/decoder.h"
#include "coap-token.h"
#include <estd/string.h>
#include "coap-uint.h"
#include "coap/context.h"

#include <estd/type_traits.h>

namespace moducom { namespace coap {

template <class TNetBufDecoder>
class option_iterator;

// standalone Decoder works well enough, so this is largely just a netbuf-capable
// wrapper around it
template <class TNetBuf>
class NetBufDecoder : public Decoder
{
public:
    typedef TNetBuf netbuf_t;

    // experimental because I am pretty sure I want netbuf_t itself to be this
    typedef estd::remove_reference<TNetBuf> netbuf_experimental_value_t;

private:
    typedef Decoder base_t;
    //typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef estd::experimental::const_buffer ro_chunk_t;

    friend class option_iterator<class TNetBufDecoder>;

protected:
    TNetBuf m_netbuf;
    // FIX: intermediate chunk until context has a value instead of a ref for its chunk
    ro_chunk_t chunk;
    Context context;


    bool process_iterate(State assert_start_state, const char* errmsg = NULLPTR)
    {
        if(errmsg)
        {
            ASSERT_ERROR(assert_start_state, state(), errmsg);
        }
        else
        {
#ifdef FEATURE_MCCOAP_CLOG
            ASSERT_ERROR(assert_start_state, state(),
                         "Expected state " << assert_start_state << " but got: " << state());
#else
            ASSERT_ERROR(assert_start_state, state(), "Encountered unexpected state");
#endif
        }
        bool result = base_t::process_iterate(context);

        return result;
    }

public:
    bool process_iterate()
    {
        // TODO: Will know how to advance through netbuf
        return base_t::process_iterate(context);
    }


    void process_option_header_experimental()
    {
        // if we're gonna evaluate an option header, ensure we're at least in option
        // processing mode
        ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

        // move past header
        process_iterate();

        ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at OptionValueStart");
    }
protected:

    // NOTE: Actually shaping up to be an internal call, because we always want
    // to move past OptionValueDone when evaluating/processing options and this
    // call does not do that
    // NOTE: May not be so useful or interesting after all, since option_number
    // and option_length are so readily available
    bool process_option_header_experimental(Option::Numbers* number, uint16_t* length)
    {
        process_option_header_experimental();

        (int&)(*number) += option_number();
        *length = option_length();

        return true;
    }

    // simulate processed memory chunk behavior here (like unprocessed,
    // but in this case const applies)
    const uint8_t* unevaluated() const
    {
        return context.chunk.data(context.pos);
    }

    // see above
    size_t length_unevaluated() const
    {
        return context.chunk.size() - context.pos;
    }

    // internal call , needs to be mated to process_option_header_experimental
public:
    // FIX: convert (reverse) this from 'partial' to 'completed' flag
    ro_chunk_t process_option_value_experimental(bool* partial = NULLPTR)
    {
        ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

        // assert that we are at ValueStart or OptionValue (latter when chunked)
        ASSERT_WARN(OptionDecoder::ValueStart, option_decoder().state(), "Must be at ValueStart");

        // NOTE: Safe to grab this, option_decoder().option_length() doesn't get clobbered for a while still
        int value_length = option_decoder().option_length();
        const uint8_t* raw = context.chunk.data() + context.pos;
        int actual_remaining_length = context.chunk.size() - context.pos;
        bool _partial = value_length > actual_remaining_length;

        if(partial != NULLPTR)
            *partial = _partial;
        else
        {
            ASSERT_WARN(false, _partial, "Partial data encountered but potentially ignored");
        }

        // it's implied there's another chunk coming if value_length
        // exceeds actual_remaining_length
        if(_partial)
            return ro_chunk_t(raw, actual_remaining_length);
        else
            return ro_chunk_t(raw, value_length);
    }


    // keep processing until we encounter state
    // keep processing for max_attempts
    // able to do this because netbuf is able to provide *all* of interested incoming buffers
    // where core decoder though it knows whether it's the last chunk, it doesn't know how
    // to retrieve the next one
    // in debug mode, this should indicate an error if we never reach said state
    // NOTE: return value convention differs here from underlying process_iterate
    //   process_iterate indicates whether we've reached end of chunk, while
    //   process_until_experimental indicates whether we found the state we're looking for
    bool process_until_experimental(Decoder::State s, int max_attempts = 10)
    {
        while(max_attempts-- > 0)
        {
            if(state() != s)
            {
                // NOTE: Probably an optimization we can inspect process iterate
                // for completion of buffer.  Don't do it yet though because
                // after buffer completion, it's still prudent to process state
                // (state iterations can happen even after buffer has been exhausted)
                process_iterate();
            }
            else
                return true;
        }

        // TODO: Reveal error that we couldn't find state s
#ifdef FEATURE_MCCOAP_CLOG
        ASSERT_ERROR(false, true, "Unable to find requested state: " << s);
#else
        ASSERT_ERROR(false, true, "Unable to find requested state");
#endif
        return false;
    }

    int8_t preprocess_token()
    {
        int8_t tkl = header_decoder().token_length();

        // assert we're starting at headerdone, and expect to move to end of TokenDone
        process_iterate(Decoder::HeaderDone);

        // won't proceed unless enough bytes have been processed to form a complete token
        // (we queue it up in this decoder)
        // if token bytes are present, we'll move through Decoder::Token state
        // if no token bytes are present, we'll move directly to Decoder::TokenDone
        if(!process_until_experimental(Decoder::TokenDone)) return -1;

        return tkl;
    }



public:
    NetBufDecoder(const netbuf_t& netbuf) :
        m_netbuf(netbuf),
        chunk(netbuf.processed(), netbuf.length_processed()),
        // NOTE: Be advised that netbuf.end() differs from traditional iterator end
        // in that it is a bool indicating that we are ON the last chunk, not PAST it
        context(chunk, netbuf.end())
    {}


    typedef coap::option_iterator<NetBufDecoder<TNetBuf> > option_iterator;

    netbuf_t& netbuf() { return m_netbuf; }

    coap::Header header()
    {
        ASSERT_WARN(Decoder::Uninitialized, state(), "Expected to be at beginning of decoder processing");

        process_until_experimental(Decoder::HeaderDone);

        return header_decoder();
    }

    void process_payload_header_experimental()
    {
        ASSERT_WARN(Decoder::OptionsDone, state(), "Expected to be at end of option processing");

        process_iterate();

        ASSERT_WARN(true,
                    state() == Decoder::Payload || state() == Decoder::Done,
                    "Did not encounter 'Paylod' or 'Done' state");
    }

    bool has_payload_experimental()
    {
        if(state() == OptionsDone)
            process_payload_header_experimental();

        // TODO: Assert that this is Payload or Done state, otherwise undefined operation
        return state() == Payload;
    }

    ro_chunk_t payload_experimental(bool* completed = NULLPTR)
    {
        if(state() == OptionsDone)
            process_payload_header_experimental();

        ASSERT_WARN(Decoder::Payload, state(), "Expected to be in payload state");

        //if(partial != NULLPTR)
          //  *partial = !netbuf().eol();

        return context.remainder();
    }

    estd::layer3::const_string payload_string_experimental(bool* completed = NULLPTR)
    {
        ro_chunk_t p = payload_experimental(completed);

        estd::layer3::const_string payload((const char*)p.data(), p.size());

        return payload;
    }

    // returns true if we consumed enough bytes to produce a complete token.  NOTE this
    //   could validly be a count of 0 bytes for 0-length tokens
    // returnse false otherwise
    bool token(layer2::Token* token)
    {
        int tkl = preprocess_token();

        if(tkl < 0) return false;

        if(tkl > 0)
            new (token) layer2::Token(token_decoder().data(), tkl);
        else
            new (token) layer2::Token(NULLPTR, 0);

        return true;
    }

    // by its nature, this will be less efficient than a layer2::Token
    // if we only hace a partial token available and conversely more efficient
    // if we have a non-partial token.  At this time Decoder (due to current TokenDecoder)
    // actually buffers incoming token, so we're talking:
    // 1. incoming packet (token present in 0 byte, fragmented, or complete form) ->
    // 2. token decoder (8-byte token buffer)
    // 3. this layer3::Token, which is a pointer to #2
    // eventually I plan to phase out mandatory #2 buffering, since the majority of use cases
    //   are a non-fragmented header+token
    layer3::Token token(bool* completed = NULLPTR)
    {
        int tkl = preprocess_token();

        if(completed != NULLPTR) *completed = tkl >= 0;

        // NOTE: We'll be assembling a layer3::Token with a -1 length here if completed isn't
        // set to true.  So a 2nd way to detect unfinished token processing, but a bit more
        // obscure
        return layer3::Token(token_decoder().data(), tkl);
    }

    // kicks off option processing
    bool begin_option_experimental()
    {
        // move us into OptionsStart, asserting we're starting at TokenDone
        process_iterate(Decoder::TokenDone);
        // move us into Options or OptionsDone
        process_iterate();

        // expected to be at DeltaAndLengthDone here
        return true;
    }


    void option_next_experimental()
    {
        // move forward past value portion / past option value start
        process_iterate();

        ASSERT_WARN(Option::OptionValueDone, option_decoder().state(), "Unexpected state");

        // if we have more options to process, sneak into 'em a bit
        if(state() != OptionsDone)
        {
            // this actually moves into domain of next option,
            // expected to be at DeltaAndLengthDone here
            process_iterate();
        }
    }

    const estd::layer3::const_string option_string_experimental(bool* partial = NULLPTR)
    {
        uint16_t length = option_decoder().option_length();

        ro_chunk_t option_value = process_option_value_experimental(partial);

        estd::layer3::const_string s((const char*)option_value.data(), length);

        return s;
    }
};


// Neat, but right way to do this would be to make a 'super' OptionsDecoder which had a bit
// of the start-stop condition awareness of full Decoder, then actually derive from that class
// so that we can do things like postfix++
template <class TNetBufDecoder>
class option_iterator
{
    typedef typename estd::remove_reference<typename TNetBufDecoder::netbuf_t>::type netbuf_t;
    typedef TNetBufDecoder decoder_t;
    typedef Option::Numbers value_type;
    typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    decoder_t& decoder;

    void partial_advance_and_get_number()
    {
        using namespace moducom::coap;

        // We presume this means there were no options
        // NOTE: might prefer that we always demand to come in with this
        // as OptionsStart and then we iterate forward ourselves
        if(decoder.state() == Decoder::OptionsDone) return;

        decoder.process_option_header_experimental();

        value_type number = (value_type) decoder.option_number();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << "Option: ";// << number;
        ::operator <<(std::clog, number); // why do I have to do this??
#endif
    }

public:
    option_iterator(decoder_t& decoder, bool begin_option = false) :
        decoder(decoder)
    {
        // NOT repeatable
        if(begin_option)
            decoder.begin_option_experimental();

        partial_advance_and_get_number();
    }

    option_iterator(DecoderContext<decoder_t>& context, bool begin_option = false) :
        decoder(context.decoder())
    {
        // NOT repeatable
        if(begin_option)
            decoder.begin_option_experimental();

        partial_advance_and_get_number();
    }


    bool valid() const
    {
        return decoder.state() == Decoder::Options;
    }

    option_iterator& operator ++()
    {
        decoder.option_next_experimental();
        if(valid())
        {
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif
            partial_advance_and_get_number();
        }
        else
        {
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif
        }

        return *this;
    }

    /* disabling postfix version because things like basic_string would be out of sync.
     * could also be problematic with chunked netbufs
    experimental_option_iterator operator ++(int)
    {
        experimental_option_iterator temp(decoder, number);
        operator ++();
        return temp;
    } */

    operator value_type()
    {
        return (value_type) decoder.option_number();
    }

    // this signature is EXPERIMENTAL.  implied by title of this class (experimental_option_iterator)
    // but this one is extra so (naming wise) than the other methods here
    ro_chunk_t opaque()
    {
        return decoder.process_option_value_experimental();
    }

    template <typename TUInt>
    TUInt uint()
    {
        // opaque will read in the entire value, no matter if the
        // int is big enough to hold it
        ro_chunk_t v = opaque();

        TUInt retval = UInt::get<TUInt>(v.data(), v.length());

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (" << retval << ')';
#endif

        return retval;
    }

    uint8_t uint8()
    {
        ro_chunk_t v = opaque();

        uint8_t retval = v.length() == 0 ? 0 : *v.data();

        ASSERT_WARN(true, v.length() <= 1, "uint8 call expects a length of <= 1 byte");

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (" << (int)retval << ')';
#endif

        return retval;
    }

    uint16_t uint16() { return uint<uint16_t>(); }
    uint32_t uint32() { return uint<uint32_t>(); }

    estd::layer3::basic_string<const char, false> string()
    {
        using namespace moducom::coap;

        estd::layer3::basic_string<const char, false> s = decoder.option_string_experimental();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (";
        ::operator <<(std::clog, s);
        std::clog << ')';
#endif
        return s;
    }

};

}}
