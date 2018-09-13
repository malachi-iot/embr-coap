//
// Created by Malachi Burke on 11/25/17.
// decoders take raw bytes as input and create meaningful output from them
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

// FIX: hacky workaround for presence of other lib's coap.h (such as with esp32)
// real fix is to inform build system not to notice their coap.h at all
#include "../coap.h"
#include "decoder/simple.h"
#include "decoder/option.h"
#include <estd/span.h>

#if __cplusplus >= 201103L
#include <type_traits>
#else
#include <estd/type_traits.h>
#endif

namespace moducom { namespace coap {


// gauruntees about contiguous nature (or lack thereof) of incoming
// messages
struct DefaultDecoderTraits
{
    static CONSTEXPR bool contiguous_through_token() { return false; }
    static CONSTEXPR bool contiguous_through_options() { return false; }

    static CONSTEXPR bool contiguous() { return false; }
};

namespace internal {

// NOTE: Running into this pattern a lot, a memory chunk augmented by a "worker position"
// which moves through it
struct DecoderContext
{
    //typedef pipeline::experimental::ReadOnlyMemoryChunk<> chunk_t;
    typedef estd::const_buffer chunk_t;

    // TODO: optimize by making this a value not a ref, and bump up "data" pointer
    // (and down length) instead of bumping up pos.  A little more fiddly, but then
    // we less frequently have to create new temporary memorychunks on the stack
    // May also just be a better architecture, so that we don't have to demand
    // a memory chunk is living somewhere for this context to operate.  Note though
    // that we need to remember what our original length was, so we still need
    // pos, unless we decrement length along the way
    const chunk_t chunk;

    // current processing position.  Should be chunk.length once processing is done
    size_t pos;

    // flag which indicates this is the last chunk to be processed for this message
    // does NOT indicate if a boundary demarkates the end of the coap message BEFORE
    // the chunk itself end
    const bool last_chunk;

    // Unused helper function
    //const uint8_t* data() const { return chunk.data() + pos; }

    chunk_t remainder() const
    {
        chunk_t r(chunk.data() + pos, chunk.size() - pos);
        return r;
        //return chunk.remainder(pos);
    }

    // Helper method as we transition to estd::const_buffer
    moducom::pipeline::MemoryChunk::readonly_t remainder_legacy() const
    {
        moducom::pipeline::MemoryChunk::readonly_t r(chunk.data() + pos, chunk.size() - pos);
        return r;
    }

public:
    DecoderContext(const chunk_t& chunk, bool last_chunk) :
            chunk(chunk),
            pos(0),
            last_chunk(last_chunk)
            {}

    DecoderContext(const pipeline::MemoryChunk::readonly_t& legacy_chunk, bool last_chunk) :
            chunk(legacy_chunk.data(), legacy_chunk.length()),
            pos(0),
            last_chunk(last_chunk)
    {}
};


}

// Incomplete DecoderBase to eventually sort out optimization of sub-decoders
// (header, token, option) based on compile-time indication of how contiguous
// our CoAP buffers are gaurunteed to be
template <class TDecoderTraits = DefaultDecoderTraits>
class DecoderBase
{
public:
    typedef TDecoderTraits decoder_traits;

#ifdef FEATURE_CPP_CONSTEXPR
    typedef typename std::conditional<
            decoder_traits::contiguous_through_token(),
            CounterDecoder<uint8_t>,
            RawDecoder<8> >::type token_decoder_t;
#else
    // pre C++11 we'll need a different technique for specialization
    typedef RawDecoder<8> token_decoder_t;
#endif


private:
    // TODO: This will become more complex as we branch out and handle decoder traits.  Note
    // that when we do, to really take advantage of optimizations, we'll have to turn Decoder::process_iterate
    // into an .hpp/templatized function
#if __cplusplus >= 201103L
    union
    {
        token_decoder_t tokenDecoder;
        OptionDecoder optionDecoder;
    };
#else
    // C++03 sensitive to unions with classes with constructors
    token_decoder_t tokenDecoder;
    OptionDecoder optionDecoder;
#endif

    // FIX: have to non-unionize headerDecoder because Decoder::process_iterate processes tokenDecoder
    // based on input from headerDecoder.  Ways to optimize this vary based on decoder_traits settings
    HeaderDecoder headerDecoder;

protected:
    DecoderBase() {}

public:
    // TODO: Make only the const ones public.  Right now a lot of the DecoderSubject
    // stuff implicitly demands non-const, but architectually speaking it should be OK
    // with const
    HeaderDecoder& header_decoder() { return headerDecoder; }
    token_decoder_t& token_decoder() { return tokenDecoder; }
    OptionDecoder& option_decoder() { return optionDecoder; }

    const HeaderDecoder& header_decoder() const { return headerDecoder; }
    const OptionDecoder& option_decoder() const { return optionDecoder; }
    const token_decoder_t& token_decoder() const { return tokenDecoder; }

protected:
    bool header_process_iterate(internal::DecoderContext& context);
    bool token_process_iterate(internal::DecoderContext& context);

    inline void init_header_decoder() { new (&header_decoder()) HeaderDecoder; }

    // NOTE: Initial reset redundant since class initializes with 0, though this
    // may well change when we union-ize the decoders.  Likely though instead we'll
    // use an in-place new
    inline void init_token_decoder() { token_decoder().reset(); }

    // NOTE: reset might be more useful if we plan on not auto-resetting
    // option decoder from within its own state machine
    inline void init_option_decoder()
    {
        new (&option_decoder()) OptionDecoder;
        //optionDecoder.reset();
    }
};


class Decoder;

namespace experimental {

template <class TSubject>
bool decode_and_notify(TSubject& subject, Decoder& decoder, internal::DecoderContext& context);

}

// TODO: As an optimization, make version of TokenDecoder which is zerocopy
class Decoder :
    public DecoderBase<DefaultDecoderTraits>,
    public internal::Root,
    public StateHelper<internal::root_state_t>
{
    template <class TMessageObserver>
    friend class DecoderSubjectBase;

    typedef internal::_root_state_t _state_t;
    typedef DecoderBase<DefaultDecoderTraits> decoder_base_t;

    // NOTE: This is necessary to use because OptionDecoder in due course of its
    // operation *might* clobber its option_number() before it fully evaluates option_length()
    // the 'length' field in optionHolder isn't *technically* necessary, as that doesn't
    // get clobbered for a while.  Leave that for an optimization.
    // Also this has the bonus of 'undeltaizing' the option number so that the consumer
    // doesn't need to track and add option deltas
    // TODO: Also be sure to union-ize this, if appropriate
    OptionDecoder::OptionExperimental optionHolder;

    // making context public (hopefully temporarily) since we use Decoder in a
    // composable (has a) vs hierarchical (is-a) way
public:
    typedef internal::DecoderContext Context;

    // FIX: Only public right now to feed experimental new estd::experimental::subject driven
    // code in subject.hpp
public:
    typedef Context::chunk_t ro_chunk_t;

    // attempt to retrieve as much of one complete option as possible
    // we assume we're positioned at the ValueStart portion of an option
    ro_chunk_t option(Context& context, bool* completed = NULLPTR);

public:
    Decoder() : StateHelper(_state_t::Uninitialized) {}

    // Available only during select times during Options state
    // FIX: May want to disambiguate naming a bit, because decoder options are starting to
    // default to side-affecting things, whereas these do not.  Considered 'peek_' prefix,
    // but that was looking pretty clumsy and ugly
    uint16_t option_number() const { return optionHolder.number_delta; }
    uint16_t option_length() const { return optionHolder.length; }

    // evaluates incoming chunk up until the next state change OR end
    // of presented chunk, then stops
    // returns true when context.chunk is fully processed, even if it is not
    // the last_chunk.  State may or may not change in this circumstance
    bool process_iterate(Context& context);

    // Of limited to no use, since we blast through chunk without caller having a chance
    // to inspect what's going on.  Only keeping around for coap_lowlevel test
    void process_deprecated(const estd::const_buffer& chunk, bool last_chunk = true)
    {
        Context context(chunk, last_chunk);

        while(!process_iterate(context));
    }

    OptionDecoder::State option_state() const 
    { 
        return option_decoder().state();
    }

    // NOTE: notify_from_decoder a bit misleading because it processes and notifies
    template <class TSubject>
    bool process_iterate_experimental(TSubject& s, Context& context)
    {
        return experimental::decode_and_notify(s, *this, context);
    }

    /// @brief process entire remainder of context, firing events via TSubject
    /// \tparam TSubject The event sink type
    /// \param s The event sink
    /// \param context
    template <class TSubject>
    void process_experimental(TSubject& s, Context& context)
    {
        while(!process_iterate_experimental(s, context));
    }
};

}}

#include "decoder.hpp"

#endif //MC_COAP_TEST_COAP_DECODER_H
