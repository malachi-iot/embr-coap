//
// Created by malachi on 12/29/17.
//
// Code that is too experimental to live alongside other code

#ifndef MC_COAP_TEST_EXPERIMENTAL_H
#define MC_COAP_TEST_EXPERIMENTAL_H

#include <mc/mem/platform.h>
#include "../coap-encoder.h"
//#include "pipeline-writer.h"

// To activate ManagedBuffer for BufferedEncoder
//#define USE_EXP_V2

namespace moducom { namespace coap { namespace experimental {

using namespace embr::coap;

namespace v2 {

// See the 2nd v2 namespace down below for more comments

struct v2_traits
{
    typedef size_t custom_size_t;
    typedef uint8_t boundary_t;
};


// TODO: Would be nice to have readonly version of this too, but not sure how practical that is
// in C++ land
// Remember: Boundaries represent the *end* of a particular block.  See current_ro
template <class TTraits = v2_traits>
class IManagedBuffer
{
public:
    typedef TTraits traits_t;
protected:
    typedef typename traits_t::custom_size_t size_t;
    typedef typename traits_t::boundary_t boundary_t;
    typedef pipeline::MemoryChunk chunk_t;
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;

public:

    // Acquire present managed buffer
    virtual const chunk_t current() const = 0;

    // Acquire portion of managed buffer, up until requested boundary (read mode)
    virtual const ro_chunk_t current_ro(boundary_t boundary = 0) const = 0;

    // Move to next managed buffer or to next current-buffer boundary
    // NOTE: Expect this to side-effect previous current() memory chunks into being invalid
    // (pointers may get reallocated, etc)
    virtual bool next() = 0;

    // resent current() to beginning
    virtual bool reset(bool reset_boundaries = false) = 0;

    // mark the current buffer at the given position with a boundary
    // the next() operation will result in the next current() operation either
    // be on the next managed buffer, or the current one just past this boundary at position
    // NOTE: Consider making this something that moves current() forward without needing a next() call
    // NOTE: It seems I made boundary() already behave this way during writes.  Instead consider
    // making this auto-return the next current()
    virtual bool boundary(boundary_t boundary, size_t position) = 0;

    // For advancing current() start pos through read or write operations
    void advance(size_t position) { boundary(0, position); }

    // NOTE: This is an attempt at simplifying api... not so effective
    // advance past last current_ro() and acquire next current_ro() up to
    // boundary
    ro_chunk_t read(boundary_t boundary)
    {
        ro_chunk_t chunk =  current_ro(boundary);
        // FIX: This is an abuse since next() is expected to possibly kill 'chunk' scope
        // however I am thinking we can get rid of next in lieu of making current_ro actually
        // do a next automatically (since it's already fast forwarding to the next boundary)
        // however that might make things complicated if we want a next() operation to HOP
        // between chunks.
        // Thinking we might still keep next() but its only application
        // would actually to be to move between physical chunks, otherwise we'd auto advance intra-chunk
        // via clever use of boundary/current_ro calls
        next();

        // Doesn't work yet because (I think)current_ro doesn't pay close enough attention to current_pos compared to
        // current_boundary
        //advance(chunk.length());
        return chunk;
    }
};


class ManagedBufferHelper
{
    typedef IManagedBuffer<> mb_t;

    mb_t& mb;
    mb_t::traits_t::custom_size_t pos;


public:
    ManagedBufferHelper(mb_t& mb) : mb(mb) {}
};


}


class BufferedEncoderBase
{
protected:
    //typedef CoAP::ParserDeprecated::State state_t;
    //typedef CoAP::ParserDeprecated _state_t;
    typedef internal::root_state_t state_t;
    typedef internal::_root_state_t _state_t;

    state_t state;
};

#ifdef UNUSED
// NOTE: very experimental.  Seems to burn up more memory and cycles than it saves
// this attempts to buffer right within IBufferedPipelineWriter itself
// TODO: Use StateHelper for this
class BufferedEncoder : public StateHelper<internal::root_state_t>
{
    //typedef CoAP::OptionExperimentalDeprecated::Numbers number_t;
    //typedef CoAP::ParserDeprecated::State state_t;
    //typedef CoAP::ParserDeprecated _state_t;
    typedef internal::option_number_t number_t;
    typedef internal::root_state_t state_t;
    typedef internal::_root_state_t _state_t;
    typedef StateHelper<state_t> base_t;


    // TODO: We could union-ize buffer and optionEncoder, since buffer
    // primarily serves header and token
    pipeline::MemoryChunk buffer;

#ifdef USE_EXP_V2
    v2::IManagedBuffer<>& mb;
#else
    pipeline::IBufferedPipelineWriter& writer;
#endif

    ExperimentalPrototypeBlockingOptionEncoder1 optionEncoder;

    // ensure header and token are advanced past
    void flush_token(state_t s)
    {
        if(state() == _state_t::Header)
        {
            size_t pos = 4 + header()->token_length();
#ifdef USE_EXP_V2
            mb.boundary(Root::boundary_segment, pos);
            mb.next();
#else
            writer.advance_write(pos, internal::Root::boundary_segment);
#endif
        }

        state(s);
    }

public:
    // acquire header for external processing
    Header* header()
    {
        return reinterpret_cast<Header*>(buffer.data());
    }


#ifdef USE_EXP_V2
    BufferedEncoder(v2::IManagedBuffer<>& mb) :
            base_t(_state_t::Header),
            mb(mb)
#else
    BufferedEncoder(pipeline::IBufferedPipelineWriter& writer) :
            base_t(_state_t::Header),
// we would prefer in this limited instance for MemoryChunk to go uninitialized,
// not a great practice but makes code more readable/efficient sometimes
#ifndef FEATURE_MC_MEM_REWRITABLE_MEMCHUNK
            // being that we can't do that in this case, zero it all out
            buffer(NULLPTR, 0),
#endif
            writer(writer)
#endif
    {
        CONSTEXPR size_t minimum_length = (sizeof(Header) + sizeof(layer1::Token));

#ifdef USE_EXP_V2
        buffer = mb.current();
#else
        buffer = writer.peek_write(minimum_length);
#endif

        header()->raw = 0;

        ASSERT_ERROR(true, buffer.length() >= minimum_length,
                     "Chunk does not meet minimum length requirements");
    }


    // acquire token position for external processing, if any
    layer1::Token* token()
    {
        return reinterpret_cast<layer1::Token*>(buffer.data() + 4);
    }


    // 100% untested
    void token(const pipeline::MemoryChunk::readonly_t& full_token)
    {
        token()->copy_from(full_token);
    }

    /*
    void token_complete()
    {
        // NOTE: Just be sure token length is assigned by the time we get here
        size_t pos = 4 + header()->token_length();
        writer.advance_write(pos);

    } */

    // option calls untested
    void option(number_t number)
    {
        flush_token(_state_t::Options);
        optionEncoder.option(writer, number);
    }


    pipeline::MemoryChunk option(number_t number, uint16_t length)
    {
        ASSERT_ERROR(true, length > 0, "Length must be > 0");

        flush_token(_state_t::Options);
#ifdef USE_EXP_V2
#error No optionEncoder which takes a write-to chunk available yet
#else
        optionEncoder.option(writer, number, length);
#endif

#ifdef USE_EXP_V2
        return mb.current();
#else
        return writer.peek_write(length);
#endif
    }


    void option(number_t number, const pipeline::MemoryChunk& chunk)
    {
        flush_token(_state_t::Options);
        optionEncoder.option(writer, number, chunk);
    }


    void payload_marker()
    {
        flush_token(_state_t::Payload);
#ifdef USE_EXP_V2
        // TODO: FIX: Need a kind of rolling/stringwriter POS behavior
        // don't necessarily have to do it IN IManagedBuffer
#error Not supported yet
#else
        writer.write(0xFF);
#endif
    }


    pipeline::MemoryChunk payload(size_t preferred_minimum_length = 0)
    {
        if(state() != _state_t::Payload)
            payload_marker();

#ifdef USE_EXP_V2
        return buffer;
#else
        return writer.peek_write(preferred_minimum_length);
#endif
    }

    void payload(const pipeline::MemoryChunk& chunk)
    {
        if(state() != _state_t::Payload)
            payload_marker();

#ifdef USE_EXP_V2
        buffer.copy_from(chunk);
        // 0-boundary experimental, we need more of an "advance"
        mb.boundary(0, chunk.length());
        mb.next();
        buffer = mb.current();
#else
        writer.write_experimental(chunk);
#endif
    }


    inline void advance(size_t advance_amount)
    {
#ifdef USE_EXP_V2
        mb.boundary(0, advance_amount);
#else
        writer.advance_write(advance_amount);
#endif
    }

    // mark encoder as complete, which in turn will mark outgoing writer
    inline void complete()
    {
#ifdef USE_EXP_V2
        mb.boundary(Root::boundary_message, 0);
#else
        writer.advance_write(0, internal::Root::boundary_message);
#endif
    }
};



// This buffers more traditionally, outside the writer
// only buffers header and token
class BufferedBlockingEncoder :
    public BlockingEncoder,
    public BufferedEncoderBase

{
    //typedef CoAP::ParserDeprecated::State state_t;
    //typedef CoAP::ParserDeprecated _state_t;
    typedef BufferedEncoderBase::state_t state_t;
    typedef BufferedEncoderBase::_state_t _state_t;

    Header _header;
    embr::coap::layer1::Token _token;

    typedef BlockingEncoder base_t;

    // ensure header and token are advanced past
    void flush_token(state_t s)
    {
        // TODO: Get rid of BufferedEncoderBase::state since BlockingEncoder
        // already has a state machine (though it's debug only so be careful)
        if(BufferedEncoderBase::state == _state_t::Header)
        {
            uint8_t token_length = _header.token_length();
            base_t::header(_header);
            if(token_length > 0)
                writer.write(_token.data(), _header.token_length());

            base_t::state(_state_t::TokenDone);
        }

        BufferedEncoderBase::state = s;
    }

public:
    BufferedBlockingEncoder(pipeline::IPipelineWriter& writer) : BlockingEncoder(writer) {}

    Header* header() { return &_header; }

    embr::coap::layer1::Token* token() { return &_token; }

    /*
    // called even if "no" token.  It should be noted that RFC 7252 says there is *always* a
    // token, even if it is a zero length token...
    void token_complete()
    {
        base_t::header(_header);
        writer.write(_token.data(), _header.token_length());
        base_t::state(_state_t::TokenDone);
    } */

    inline void option(option_number_t number, const pipeline::MemoryChunk& value)
    {
        flush_token(_state_t::Options);
        base_t::option(number, value);
    }

    inline void option(option_number_t number)
    {
        flush_token(_state_t::Options);
        base_t::option(number);
    }

    inline void option(option_number_t number, const char* str)
    {
        flush_token(_state_t::Options);
        base_t::option(number, str);
    }

};
#endif

template <typename custom_size_t = size_t>
class ProcessedMemoryChunkBase
{
protected:
    custom_size_t _processed;

public:
    ProcessedMemoryChunkBase(custom_size_t p = 0) : _processed(p) {}

    custom_size_t processed() const { return _processed; }
    void processed(custom_size_t p) { _processed = p; }
};

// length = total buffer size
// processed = "valid" buffer size, size of buffer actually used
// NOTE: These seem clumsy.  They are supposed to expose more of a 'memorychunk'
// interface in conjunction with a processed memory chunk, but instead it just
// seems confusing
class ProcessedMemoryChunk :
    public pipeline::MemoryChunk,
    public ProcessedMemoryChunkBase<>
{
public:
    ProcessedMemoryChunk(const pipeline::MemoryChunk& chunk) : pipeline::MemoryChunk(chunk) {}

};


namespace layer1 {

template <size_t buffer_size, typename custom_size_t = size_t>
class ProcessedMemoryChunk :
    public pipeline::layer1::MemoryChunk<buffer_size>,
    public ProcessedMemoryChunkBase<custom_size_t>
{
public:
};

}


namespace layer2 {

template <size_t buffer_size, typename custom_size_t = size_t>
class ProcessedMemoryChunk :
        public pipeline::layer2::MemoryChunk<buffer_size, custom_size_t>,
        public ProcessedMemoryChunkBase<custom_size_t>
{
public:
};

}


void process_header_request(Header request, Header* response);


// NOT USED just capturing idea that pipeline reader and pipeline writer might actually be 99% the same
template <class TMessage, typename custom_size_t = size_t>
class IPipeline2
{
    typedef uint8_t boundary_t;

public:
    virtual TMessage current() = 0;

    virtual bool next() = 0;

    virtual custom_size_t advance(custom_size_t length, boundary_t boundary) = 0;
};


namespace v2 {
// NOTE:
// Ultimately we will have 3 classes of data streaming/pipelining:
// 1) classic stream-style copy-oriented, augmented by "boundary" capability
// 2) normal, in which case pipeline "writer" will maintain buffer
// 3) buffered, in which case pipeline itself maintains buffer
//
// right now code architecture is a mess and does not reflect or truly support this

// named experimental::v2 because we have so many experimental things already , so this is current
// newest one

template <class TTraits = v2_traits>
class IStreamReader
{
    typedef TTraits traits_t;
    typedef typename traits_t::custom_size_t size_t;
    typedef typename traits_t::boundary_t boundary_t;

public:
    // number of bytes available to read, up to boundary condition if specified
    virtual size_t available(boundary_t boundary = 0) = 0;

    // reads read_into.length(), available() or boundary-condition-met bytes, whichever arrives
    // first.  return value reflects bytes read
    // TODO: May also need a way to reflect if boundary is hit
    virtual size_t read(const pipeline::MemoryChunk& read_into, boundary_t boundary = 0) = 0;
};


template <class TTraits = v2_traits>
class IPipelineReader : public IPipeline2<pipeline::PipelineMessage, typename TTraits::custom_size_t>
{
public:

};



template <class TTraits = v2_traits>
class IStreamWriter
{
    typedef TTraits traits_t;
    typedef typename traits_t::custom_size_t size_t;
    typedef typename traits_t::boundary_t boundary_t;

public:
    // how many non-blockable bytes we can write
    virtual size_t writeable() = 0;

    // writes write_from.length(), writeable() whichever comes first.  Demarkates
    // the "end" of this write with boundary
    virtual size_t write(const pipeline::MemoryChunk::readonly_t& write_from, boundary_t boundary = 0) = 0;
};

template <class TTraits = v2_traits>
class IPipelineWriter
{
    typedef TTraits traits_t;
    typedef typename traits_t::custom_size_t size_t;
    typedef typename traits_t::boundary_t boundary_t;

public:
    // returns true if we can register any more messages
    virtual bool writeable() = 0;

    // registers a specified pipeline message.  For now, we still utilize
    // PipelineMessage::CopiedStatus::boundary field, but starting to feel like
    // that might be phased out ... ?  at least in this context
    virtual bool write(const pipeline::PipelineMessage& register_message) = 0;
};



class ManagedBuffer : public IManagedBuffer<>
{
    struct BoundaryDescriptor
    {
        boundary_t boundary;
        size_t pos;
    };

    pipeline::layer1::MemoryChunk<1024> chunk;
    BoundaryDescriptor boundaries[10];
    uint8_t boundary_count;
    uint8_t current_boundary; // when doing a read, this is what boundary (in boundaries) we're up to
    size_t current_pos; // shared between read and write at the moment

public:
    ManagedBuffer();

    virtual const chunk_t current() const OVERRIDE;

    virtual const ro_chunk_t current_ro(boundary_t boundary = 0) const OVERRIDE;

    virtual bool next() OVERRIDE;

    virtual bool reset(bool reset_boundaries = false) OVERRIDE;

    virtual bool boundary(boundary_t boundary, size_t position) OVERRIDE;
};


}


class Obstack : public dynamic::IMemory
{
public:
};


}}}

#endif //MC_COAP_TEST_EXPERIMENTAL_H
