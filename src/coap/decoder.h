/**
 * @file
 * @author  Malachi Burke
 * @date    11/25/17
 * @brief
 * decoders take raw bytes as input and create meaningful output from them
 */
// Created by Malachi Burke on 11/25/17.
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

// FIX: hacky workaround for presence of other lib's coap.h (such as with esp32)
// real fix is to inform build system not to notice their coap.h at all
#include "../coap.h"
#include "decoder/simple.h"
#include "decoder/option.h"
#include "decoder/context.h"
#include "decoder/decode-and-notify.h"

#if __cplusplus >= 201103L
#include <type_traits>
#else
#include <estd/type_traits.h>
#endif

// for option decode chunk processing, skip the linger on OptionLengthDone and
// OptionDeltaAndLengthDone since ValueStart always appears at the end of those
// cycles
// NOTE: Almost works, but seems to upset old NetBuf/option iterator code.
// Since it's a priority to zap those first, waiting on this
#define FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE    1

namespace embr { namespace coap {


// gauruntees about contiguous nature (or lack thereof) of incoming
// messages
struct DefaultDecoderTraits
{
    static CONSTEXPR bool contiguous_through_token() { return false; }
    static CONSTEXPR bool contiguous_through_options() { return false; }

    static CONSTEXPR bool contiguous() { return false; }
};

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
    // Not yet active, if it's safe after option phase is done we can use this
    // to emit some useful data on completion event
    struct CompletionState
    {
        bool payloadPresent;
    };

    // TODO: This will become more complex as we branch out and handle decoder traits.  Note
    // that when we do, to really take advantage of optimizations, we'll have to turn Decoder::process_iterate
    // into an .hpp/templatized function
#if __cplusplus >= 201103L
    union
    {
        token_decoder_t tokenDecoder;
        OptionDecoder optionDecoder;
        CompletionState completionState;
    };
#else
    // C++03 sensitive to unions with classes with constructors
    token_decoder_t tokenDecoder;
    OptionDecoder optionDecoder;
    CompletionState completionState;
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
    const CompletionState& completion_state() const { return completionState; }

protected:
    bool header_process_iterate(internal::DecoderContext& context);
    bool token_process_iterate(internal::DecoderContext& context);

    inline void init_completion_state(bool payloadPresent)
    {
        new (&completionState) CompletionState;
        completionState.payloadPresent = payloadPresent;
    }

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

protected:
    void process_iterate_internal();

    // NOTE: This is necessary to use because OptionDecoder in due course of its
    // operation *might* clobber its option_number() before it fully evaluates option_length()
    // the 'length' field in optionHolder isn't *technically* necessary, as that doesn't
    // get clobbered for a while.  Leave that for an optimization.
    // Also this has the bonus of 'undeltaizing' the option number so that the consumer
    // doesn't need to track and add option deltas
    // TODO: Also be sure to union-ize this, if appropriate
    OptionDecoder::Holder optionHolder;

    // making context public (hopefully temporarily) since we use Decoder in a
    // composable (has a) vs hierarchical (is-a) way
public:
    typedef internal::DecoderContext Context;

    // FIX: Only public right now to feed experimental new estd::experimental::subject driven
    // code in subject.hpp
public:
    typedef Context::chunk_type ro_chunk_t;

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
    ///
    /// \param context
    /// \return true when end of context is reached, false otherwise
    iterated::decode_result process_iterate(Context& context);

    OptionDecoder::State option_state() const
    { 
        return option_decoder().state();
    }
};

}}

// NOTE: Reverse this?  decoder.hpp includes decoder.h?
#include "decoder.hpp"

#endif //MC_COAP_TEST_COAP_DECODER_H
