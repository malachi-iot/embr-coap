#pragma once

#include "../coap-dispatcher.h"

namespace moducom { namespace coap {

// In-progress: revamped "Dispatcher"
template <class TMessageObserver>
class DecoderSubjectBase
{
    Decoder decoder;
    TMessageObserver observer;
    typedef experimental::option_number_t option_number_t;

    // do these observer_xxx versions so that compile errors are easier to track
    inline void observer_on_option(option_number_t n,
                                   const pipeline::MemoryChunk::readonly_t& optionChunk,
                                   bool last_chunk)
    {
        observer.on_option(n, optionChunk, last_chunk);
    }


    inline void observer_on_option(option_number_t n, uint16_t len)
    {
        observer.on_option(n, len);
    }


    inline void observer_on_header(const Header& header)
    {
        observer.on_header(header);
    }


    inline void observer_on_token(const pipeline::MemoryChunk::readonly_t& token, bool last_chunk)
    {
        observer.on_token(token, last_chunk);
    }

    inline void observer_on_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk)
    {
        observer.on_payload(payloadChunk, last_chunk);
    }

    // returns false while chunk/context has not been exhausted
    // returns true once it has
    bool dispatch_iterate(Decoder::Context& context);

    void dispatch_header();
    void dispatch_token();

    // optionChunk is a subset/processed version of dispatch(chunk)
    size_t dispatch_option(const pipeline::MemoryChunk::readonly_t& optionChunk);
    void dispatch_payload(const pipeline::MemoryChunk::readonly_t& payloadChunk, bool last_chunk);

public:
    DecoderSubjectBase(TMessageObserver observer) : observer(observer) {}
    DecoderSubjectBase(IncomingContext& context) : observer(context) {}

    // returns number of bytes processed from chunk
    size_t dispatch(const pipeline::MemoryChunk::readonly_t& chunk, bool last_chunk = true)
    {
        Decoder::Context context(chunk, last_chunk);

        while(!dispatch_iterate(context) && decoder.state() != Decoder::Done);

        return context.pos;
    }
};

}}