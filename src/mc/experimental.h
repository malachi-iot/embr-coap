//
// Created by malachi on 12/29/17.
//
// Code that is too experimental to live alongside other code

#ifndef MC_COAP_TEST_EXPERIMENTAL_H
#define MC_COAP_TEST_EXPERIMENTAL_H

#include "../platform.h"
#include "../coap-encoder.h"
#include "pipeline-writer.h"


namespace moducom { namespace coap { namespace experimental {

// NOTE: very experimental.  Seems to burn up more memory and cycles than it saves
// this attempts to buffer right within IBufferedPipelineWriter itself
class BufferedEncoder
{
    typedef CoAP::OptionExperimental::Numbers number_t;

    pipeline::IBufferedPipelineWriter& writer;
    pipeline::MemoryChunk buffer;

public:
    BufferedEncoder(pipeline::IBufferedPipelineWriter& writer) : writer(writer)
    {
        CONSTEXPR size_t minimum_length = (sizeof(CoAP::Header) + sizeof(layer1::Token));

        buffer = writer.peek_write(minimum_length);

        ASSERT_ERROR(true, buffer._length() >= minimum_length,
                     "Chunk does not meet minimum length requirements");
    }

    // acquire header for external processing
    CoAP::Header* header()
    {
        return reinterpret_cast<CoAP::Header*>(buffer.data);
    }

    // acquire token position for external processing, if any
    layer1::Token* token()
    {
        return reinterpret_cast<layer1::Token*>(buffer.data + 4);
    }

    void token_complete()
    {
        // NOTE: Just be sure token length is assigned by the time we get here
        size_t pos = 4 + header()->token_length();
        writer.advance_write(pos);
    }

    // TODO: Do options just like regular BlockingEncoder, since they are actually very well suited
    // to streaming.  Their option-values may or may not be , but probably they are
    void option(number_t number)
    {
        // TODO
    }


    void payload_marker()
    {
        writer.write(0xFF);
    }
};



// This buffers more traditionally, outside the writer
// only buffers header and token
class BufferedBlockingEncoder : public BlockingEncoder
{
    CoAP::Header _header;
    moducom::coap::layer1::Token _token;

    typedef BlockingEncoder base_t;

public:
    BufferedBlockingEncoder(pipeline::IPipelineWriter& writer) : BlockingEncoder(writer) {}

    CoAP::Header* header() { return &_header; }

    moducom::coap::layer1::Token* token() { return &_token; }

    // called even if "no" token.  It should be noted that RFC 7252 says there is *always* a
    // token, even if it is a zero length token...
    void token_complete()
    {
        base_t::header(_header);
        writer.write(_token.data(), _header.token_length());
        state(_state_t::TokenDone);
    }
};


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


}}}

#endif //MC_COAP_TEST_EXPERIMENTAL_H
