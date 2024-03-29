/**
 * @file
 * @author  Malachi Burke
 * 
 * 12/3/2019: OBSOLETE
 * Basically this is netbuf mk. 1 code.  The option_iterator is an interesting
 * mechanism however we might want to bring that forward somehow
 */

#pragma once

#include "coap/decoder.h"
#include "../token.h"
#include <estd/string.h>
#include <estd/optional.h>
#include "../uint.h"
#include "coap/context.h"

#include <estd/type_traits.h>
#include <estd/utility.h> // for ESTD_FN_HAS_METHOD

namespace embr { namespace coap {

template <class TNetBufDecoder>
class option_iterator;

// Split out code from NetBufDecoder, however, may have been a bit overzealous
// as the NetBufDecoder doesn't fully do the next/advance semantics just yet,
// and therefore some of the calls I moved into DecoderWithContext may have
// to be overridden/moved back to NetBufDecoder
class DecoderWithContext : public Decoder
{
public:
    // FIX: Want this to be private.  Making this public only so that we can easily experiment
    // with notify_from_decoder which currently is a global function, but probably eventually
    // will be a member function of Decoder
    Context context;

protected:
    typedef Decoder base_t;
    typedef internal::const_buffer ro_chunk_t;

    DecoderWithContext(const ro_chunk_t& chunk, bool last_chunk) :
        context(chunk, last_chunk)
        {}

    // simulate processed memory chunk behavior here (like unprocessed,
    // but in this case const applies)
    const uint8_t* unevaluated() const
    {
        return context.chunk.data() + context.pos;
    }

    // see above
    size_t length_unevaluated() const
    {
        return context.chunk.size() - context.pos;
    }


    iterated::decode_result process_iterate(State assert_start_state, const char* errmsg = NULLPTR)
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
        return base_t::process_iterate(context);
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

    // evaluate if we are on TokenStart or TokenDone
    // (new utilization of existing state machine we eagerly
    //  move to next 'start')
    estd::layer1::optional<uint8_t, 255> preprocess_token()
    {
        // escape route for no-token scenario.  We actually blast by TokenDone
        // so also OptionsStart would abort scenario.   All these abort scenarios
        // produce null/empty tokens which loosely corresponds to spec (spec
        // indicates a token is always present.... even if it's 0 length)
        // Because it's a spec thing, we'll not issue warnings when generating
        // a 0 length token here
        if(state() == Decoder::TokenDone) return 0;
        if(state() == Decoder::OptionsStart) return 0;

        // hold onto tkl here because once we start token processing, token_decoder() may clobber
        // the value
        estd::layer1::optional<uint8_t, 255> tkl = header_decoder().token_length();

        // assert we're starting at TokenStart, and expect to move to end of TokenDone
        process_iterate(Decoder::TokenStart);

        // won't proceed unless enough bytes have been processed to form a complete token
        // (we queue it up in this decoder)
        // if token bytes are present, we'll move through Decoder::Token state
        // if no token bytes are present, we'll move directly to Decoder::TokenDone
        if(!process_until_experimental(Decoder::TokenDone)) return estd::nullopt;

        process_iterate();

        return tkl;
    }


public:
    iterated::decode_result process_iterate()
    {
        // TODO: Will know how to advance through netbuf
        return base_t::process_iterate(context);
    }



    coap::Header header()
    {
        ASSERT_WARN(Decoder::Uninitialized, state(), "Expected to be at beginning of decoder processing");

        process_until_experimental(Decoder::HeaderDone);

        coap::Header header = header_decoder();

        // will move us to TokenStart if token is present, or TokenDone immediately
        // if no token is present
        process_iterate();

        // if we do arrivate at TokenDone (no token),
        // move right by it into OptionsStart.  Consumers should know based on header
        // whether any token consumption is necessary - and if for some reason they don't,
        // they can fall back and see if we're at TokenStart or OptionsStart
        if(state() == Decoder::TokenDone)
            process_iterate();

        return header;
    }

    // returns true if we consumed enough bytes to produce a complete token.  NOTE this
    //   could validly be a count of 0 bytes for 0-length tokens
    // returnse false otherwise
    bool token(layer2::Token* token)
    {
        // -1/no value means only partial token available
        estd::layer1::optional<uint8_t, 255> tkl = preprocess_token();

        if(tkl)
            new (token) layer2::Token(token_decoder().data(), *tkl);

        // after yanking out token, this will move us from OptionsStart
        // to either Options or OptionsDone.  More eagerness, so that
        // we're 100% positioned on any interesting option(s)
        //process_iterate();

        return tkl;
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
        estd::layer1::optional<uint8_t, 255> tkl = preprocess_token();

        if(completed != NULLPTR) *completed = tkl.has_value();

        // NOTE: May want to pass in a layer3::Token* instead, so that we
        // can return false like above
        layer3::Token t(token_decoder().data(), tkl ? *tkl : 0);

        // cannot embark into options here because it clobbers token_decoder().data()
        //process_iterate();

        return t;
    }


    // semi-experimental, could be fat copying ctx around.  However,
    // pretty sure RVO will kick in
    TokenAndHeaderContext<true, false> header_and_token()
    {
        TokenAndHeaderContext<true, false> ctx;

        ctx.header(header());
        ctx.token(token());

        return ctx;
    }

    ro_chunk_t option(bool* completed = NULLPTR)
    {
        return base_t::option(context, completed);
    }
};


#define FEATURE_MCCOAP_EMBR_NETBUF

// standalone Decoder works well enough, so this is largely just a netbuf-capable
// wrapper around it
template <class TNetBuf>
class NetBufDecoder : public DecoderWithContext
{
public:
    typedef typename estd::remove_reference<TNetBuf>::type netbuf_t;

private:
    typedef DecoderWithContext base_t;
    //typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    friend class option_iterator<class TNetBufDecoder>;

protected:
    TNetBuf m_netbuf;

public:
    void process_option_header_experimental()
    {
        // if we're gonna evaluate an option header, ensure we're at least in option
        // processing mode
        ASSERT_WARN(Decoder::Options, state(), "Must be in options processing mode");

        // move past header
        process_iterate();

        // FIX: This could break if chunk boundary hits us in the middle of the number
        // & value-length.  Should be as simple as repeating the process_iterate with
        // the next context chunk populated from netbuf
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

#ifdef UNIT_TESTING
public:
#endif
    // helpers to detect new embr style netbuf or not
    ESTD_FN_HAS_METHOD(void*, data,)

    ESTD_FN_HAS_METHOD(bool, end,)

    // internal call , needs to be mated to process_option_header_experimental
public:



public:
#ifdef FEATURE_MCCOAP_EMBR_NETBUF
    // for legacy coap-netbuf version
    template <class TNetBuf2,
                class = typename estd::enable_if<has_end_method<
                            typename estd::remove_reference<TNetBuf2>::type
                        >::value>::type>
    NetBufDecoder(TNetBuf2& netbuf) :
        // NOTE: Be advised that netbuf.end() differs from traditional iterator end
        // in that it is a bool indicating that we are ON the last chunk, not PAST it
        base_t(ro_chunk_t(netbuf.processed(), netbuf.length_processed()), netbuf.end()),
        m_netbuf(netbuf)
    {}

    // for new embr-netbuf version
    template <class TNetBuf2,
            class = typename estd::enable_if<
                    has_data_method<
                            typename estd::remove_reference<TNetBuf2>::type
                        >
                        ::value>
                    ::type>
    NetBufDecoder(TNetBuf2& netbuf, bool = true) :
    // NOTE: Be advised that netbuf.end() differs from traditional iterator end
    // in that it is a bool indicating that we are ON the last chunk, not PAST it
            base_t(ro_chunk_t((uint8_t*)netbuf.data(), netbuf.size()),
                   netbuf.last()),
            m_netbuf(netbuf)
    {}

    template <ptrdiff_t N>
    NetBufDecoder(const uint8_t (&array)[N]) :
        base_t(ro_chunk_t(array, N), true),
        m_netbuf(array)
    {

    }
#else
    NetBufDecoder(const netbuf_t& netbuf) :
    // NOTE: Be advised that netbuf.end() differs from traditional iterator end
    // in that it is a bool indicating that we are ON the last chunk, not PAST it
            base_t(ro_chunk_t(netbuf.processed(), netbuf.length_processed()), netbuf.end()),
            m_netbuf(netbuf)
    {}
#endif


    typedef coap::option_iterator<NetBufDecoder<TNetBuf> > option_iterator;

    netbuf_t& netbuf() { return m_netbuf; }

#ifdef UNUSED
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
#endif

    // NOTE: since context only reveals one buffer's worth of information,
    // at this point we can't know if a payload is chunked or not
    ro_chunk_t payload()
    {
#ifdef UNUSED
        if(state() == OptionsDone)
            process_payload_header_experimental();
#endif

        ASSERT_WARN(Decoder::Payload, state(), "Expected to be in payload state");

        return context.remainder();
    }


    // kicks off option processing.  We do this explicitly so that
    // the layer3::Token may be used (because initializing the
    // options decoder will clobber it)
    bool begin_option_experimental()
    {
        // we should be at OptionsStart here
        // move us into Options or OptionsDone
        process_iterate(Decoder::OptionsStart);

        // if we zoom right to options done, then no options are present
        if(state() == Decoder::OptionsDone)
        {
            // with no options present, move straight away to payload or
            // done
            process_iterate();
            return false; // indicate no options present
        }

        // expected to be at DeltaAndLengthDone here
        return true; // indicate options are present
    }


    ///
    /// \brief continues with option processing
    ///
    /// Must be preceded by option_begin
    ///
    /// \return true if another option is available, false otherwise
    ///
    bool option_next()
    {
        // move forward past value portion / past option value start
        process_iterate(Decoder::Options);

        ASSERT_WARN(Option::OptionValueDone, option_decoder().state(), "Unexpected state");

        // if we have more options to process, sneak into 'em a bit
        // this actually moves into domain of next option,
        // expected to be at DeltaAndLengthDone here
        process_iterate();
        // if this marks the end of option processing, eagerly move into
        // next state which will be either Payload or Done
        if(state() == OptionsDone)
            process_iterate();

        return state() == Decoder::Options;
    }
};


// Neat, but right way to do this would be to make a 'super' OptionsDecoder which had a bit
// of the start-stop condition awareness of full Decoder, then actually derive from that class
// so that we can do things like postfix++
template <class TNetBufDecoder>
class option_iterator
{
    typedef TNetBufDecoder decoder_t;
    typedef typename decoder_t::netbuf_t netbuf_t;
    typedef Option::Numbers value_type;
    typedef internal::const_buffer ro_chunk_t;

    decoder_t& decoder;

public:
    Option::Numbers number() const
    {
        return static_cast<Option::Numbers>(decoder.option_number());
    }

private:
    void partial_advance_and_get_number()
    {
        using namespace embr::coap;

        // chew on and decode option number and length
        decoder.process_option_header_experimental();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << "Option: ";// << number;
        ::operator <<(std::clog, number()); // why do I have to do this??
#endif
    }

public:
    option_iterator(decoder_t& decoder) :
        decoder(decoder)
    {
        if(decoder.begin_option_experimental())
            partial_advance_and_get_number();
    }

    option_iterator(DecoderContext<decoder_t>& context) :
        decoder(context.decoder())
    {
        if(decoder.begin_option_experimental())
            partial_advance_and_get_number();
    }


    bool valid() const
    {
        return decoder.state() == Decoder::Options;
    }

    ///
    /// \brief Specifically there is no postfix version
    ///
    /// ... since this aliases out to an underlying state machine whose state
    /// includes a potentially forward-only netbuf
    ///
    /// \return
    ///
    option_iterator& operator ++()
    {
        bool valid = decoder.option_next();
        if(valid)
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

    operator value_type() const
    {
        return number();
    }

    ro_chunk_t opaque(bool* completed = NULLPTR) const
    {
        return decoder.option(completed);
    }

    // NOTE: if this gets chunked, right now you'll have to manually reassemble
    // it
    template <typename TUInt>
    TUInt uint(bool* completed = NULLPTR) const
    {
        // opaque will read in the entire value, no matter if the
        // int is big enough to hold it
        ro_chunk_t v = opaque(completed);

        TUInt retval = UInt::get<TUInt>(v);

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (" << retval << ')';
#endif

        return retval;
    }

    uint8_t uint8() const
    {
        ro_chunk_t v = opaque();

        uint8_t retval = v.size() == 0 ? 0 : *v.data();

        ASSERT_WARN(true, v.size() <= 1, "uint8 call expects a length of <= 1 byte");

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (" << (int)retval << ')';
#endif

        return retval;
    }

    uint16_t uint16() const { return uint<uint16_t>(); }
    uint32_t uint32() const { return uint<uint32_t>(); }

    estd::layer3::const_string string(bool* completed = NULLPTR) const
    {
        estd::layer3::const_string s = opaque(completed);

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (";
        ::operator <<(std::clog, s);
        std::clog << ')';
#endif
        return s;
    }

};

}}
