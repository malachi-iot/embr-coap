#pragma once

#include <estd/span.h>

#include <mc/memory-chunk.h>

// This is the legacy way of pushing data through our decoder.  It has the advantage
// that is that it is dead simple while still being relatively powerful owing to the
// 'last_chunk' flag.  And C++03 friendly.  Our streambufs are great, but they are not that.
// So we keep it
namespace embr { namespace coap { namespace internal {

// NOTE: Running into this pattern a lot, a memory chunk augmented by a "worker position"
// which moves through it.
template <class TChar>
struct DataContextBase
{
    typedef estd::span<TChar> chunk_type;

    // TODO: optimize by making this a value not a ref, and bump up "data" pointer
    // (and down length) instead of bumping up pos.  A little more fiddly, but then
    // we less frequently have to create new temporary memorychunks on the stack
    // May also just be a better architecture, so that we don't have to demand
    // a memory chunk is living somewhere for this context to operate.  Note though
    // that we need to remember what our original length was, so we still need
    // pos, unless we decrement length along the way
    const chunk_type chunk;

    // current processing position.  Should be chunk.length once processing is done
    size_t pos;

    // flag which indicates this is the last chunk to be processed for this message
    // does NOT indicate if a boundary demarkates the end of the coap message BEFORE
    // the chunk itself end
    const bool last_chunk;

    // Unused helper function
    //const uint8_t* data() const { return chunk.data() + pos; }

    chunk_type remainder() const
    {
        return chunk_type(chunk.data() + pos, chunk.size() - pos);
    }

    // Helper method as we transition to estd::const_buffer
    moducom::pipeline::MemoryChunk::readonly_t remainder_legacy() const
    {
        moducom::pipeline::MemoryChunk::readonly_t r(chunk.data() + pos, chunk.size() - pos);
        return r;
    }

public:
    DataContextBase(const chunk_type& chunk, bool last_chunk) :
        chunk(chunk),
        pos(0),
        last_chunk(last_chunk) {}

    DataContextBase(const moducom::pipeline::MemoryChunk::readonly_t& legacy_chunk, bool last_chunk) :
        chunk(legacy_chunk.data(), legacy_chunk.length()),
        pos(0),
        last_chunk(last_chunk) {}
};

typedef DataContextBase<const uint8_t> DecoderContext;

}

namespace iterated {

// DEBT: Temporarily putting this here as we flesh out this upgraded return type
struct decode_result
{
    /// End of data stream was reached.  NOTE that this is not considered
    /// a stopping point, as further state machine processing may need to
    /// occur on gathered data.  Look to 'done'
    uint16_t eof : 1;
    /// Data is in an unavailable/unknown state
    uint16_t waitstate : 1;
    /// CoAP decoding encountered an error - not yet active
    uint16_t failure : 1;
    /// CoAP completed decoding the packet normally
    uint16_t done: 1;
};

}

}}