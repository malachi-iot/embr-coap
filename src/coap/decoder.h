//
// Created by Malachi Burke on 11/25/17.
// decoders take raw bytes as input and create meaningful output from them
//

#ifndef MC_COAP_TEST_COAP_DECODER_H
#define MC_COAP_TEST_COAP_DECODER_H

// FIX: hacky workaround for presence of other lib's coap.h (such as with esp32)
// real fix is to inform build system not to notice their coap.h at all
#include "../coap.h"
#include "coap/decoder/simple.h"
#include "coap/decoder/option.h"

namespace moducom { namespace coap {

// TODO: As an optimization, make version of TokenDecoder which is zerocopy
class Decoder :
    public internal::Root,
    public StateHelper<internal::root_state_t>
{
    typedef internal::_root_state_t _state_t;

protected:
    // TODO: Union-ize this  Not doing so now because of C++03 trickiness
    HeaderDecoder headerDecoder;
    OptionDecoder optionDecoder;
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
    HeaderDecoder& header_decoder() { return headerDecoder; }
    TokenDecoder& token_decoder() { return tokenDecoder; }
    OptionDecoder& option_decoder() { return optionDecoder; }

    // making context public (hopefully temporarily) since we use Decoder in a
    // composable (has a) vs hierarchical (is-a) way
public:
    // NOTE: Running into this pattern a lot, a memory chunk augmented by a "worker position"
    // which moves through it
    struct Context
    {
        typedef pipeline::experimental::ReadOnlyMemoryChunk<> chunk_t;

        // TODO: optimize by making this a value not a ref, and bump up "data" pointer
        // (and down length) instead of bumping up pos.  A little more fiddly, but then
        // we less frequently have to create new temporary memorychunks on the stack
        // May also just be a better architecture, so that we don't have to demand
        // a memory chunk is living somewhere for this context to operate.  Note though
        // that we need to remember what our original length was, so we still need
        // pos, unless we decrement length along the way
        const chunk_t& chunk;

        // current processing position.  Should be chunk.length once processing is done
        size_t pos;

        // flag which indicates this is the last chunk to be processed for this message
        // does NOT indicate if a boundary demarkates the end of the coap message BEFORE
        // the chunk itself end
        const bool last_chunk;

        // Unused helper function
        //const uint8_t* data() const { return chunk.data() + pos; }

        chunk_t remainder() const { return chunk.remainder(pos); }

    public:
        Context(const chunk_t& chunk, bool last_chunk) :
                chunk(chunk),
                pos(0),
                last_chunk(last_chunk)
                {}
    };

    // NOTE: This is necessary to use because OptionDecoder in due course of its
    // operation *might* clobber its option_number() before it fully evaluates option_length()
    // the 'length' field in optionHolder isn't *technically* necessary, as that doesn't
    // get clobbered for a while.  Leave that for an optimization.
    // Also this has the bonus of 'undeltaizing' the option number so that the consumer
    // doesn't need to track and add option deltas
    // TODO: Also be sure to union-ize this, if appropriate
    OptionDecoder::OptionExperimental optionHolder;


public:
    Decoder() : StateHelper(_state_t::Uninitialized) {}

    // Available only during select times during Options state
    uint16_t option_number() const { return optionHolder.number_delta; }
    uint16_t option_length() const { return optionHolder.length; }

    // evaluates incoming chunk up until the next state change OR end
    // of presented chunk, then stops
    // returns true when context.chunk is fully processed, even if it is not
    // the last_chunk.  State may or may not change in this circumstance
    bool process_iterate(Context& context);

    // Of limited to no use, since we blast through chunk without caller having a chance
    // to inspect what's going on
    void process(const pipeline::experimental::ReadOnlyMemoryChunk<>& chunk, bool last_chunk = true)
    {
        Context context(chunk, last_chunk);

        while(!process_iterate(context));
    }

    OptionDecoder::State option_state() const 
    { 
        return optionDecoder.state(); 
    }
};

#ifdef UNUSED

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
        size_t length = msg.length();

        while(length--)
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

    const Header& header() const { return decoder; }
};

/*
inline bool decode_to_input_iterate(PipelineReaderDecoder<OptionDecoder>& decoder, pipeline::IBufferedPipelineReader& reader, IOptionObserver& input)
{
    decoder.process_iterate(reader);
}
*/
}
#endif

}}

#endif //MC_COAP_TEST_COAP_DECODER_H
