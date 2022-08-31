/**
 * @file
 * @author  Malachi Burke
 */

#pragma once

#include <coap/decoder.h>
#include <coap/decoder/streambuf.h>
#include <obsolete/coap-dispatcher.h>

#include <estd/span.h>
#include <exp/events.h>
#include <coap/decoder/observer/util.h>

// Just before we went with on_notify approach we had a forming useful
// on_payload, on_token etc. approach.  That approach was more built out
// with some interesting experimental features, but is now displaced by
// 'decode_and_notify'.
#ifndef FEATURE_MCCOAP_LEGACY_EVENTS
#define FEATURE_MCCOAP_LEGACY_EVENTS 1
#endif

namespace embr { namespace coap {

#if FEATURE_MCCOAP_LEGACY_EVENTS
// Revamped "Dispatcher"
// Now passing basic test suite
template <class TMessageObserver>
class DecoderSubjectBase
{
    Decoder decoder;
    TMessageObserver observer;
    typedef internal::option_number_t option_number_t;
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef estd::const_buffer ro_chunk2_t;

    // do these observer_xxx versions so that compile errors are easier to track
    inline void observer_on_option(option_number_t n,
                                   const ro_chunk_t& optionChunk,
                                   bool last_chunk)
    {
        observer.on_option(n, optionChunk, last_chunk);
    }


    // yanks info from decoder.option_number and decoder.option_length
    void observer_on_option(const ro_chunk_t& option_chunk);


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

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    inline void observer_on_complete()
    {
        observer.on_complete();
    }
#endif

    // returns false while chunk/context has not been exhausted
    // returns true once it has
    iterated::decode_result dispatch_iterate(Decoder::Context& context);

    void dispatch_token();
    void dispatch_option(Decoder::Context& context);

public:
    TMessageObserver& get_observer() { return observer; }

    DecoderSubjectBase(TMessageObserver observer) : observer(observer) {}

    template <class TIncomingContext>
    DecoderSubjectBase(TIncomingContext& context) : observer(context) {}

    // returns number of bytes processed from chunk
    /**
     * @brief dispatch
     * @param chunk
     * @param last_chunk denotes whether we have reached complete end of coap message with this chunk
     * @return
     */
    size_t dispatch(const ro_chunk2_t& chunk, bool last_chunk = true)
    {
        Decoder::Context context(chunk, last_chunk);

        while(!dispatch_iterate(context).eof && decoder.state() != Decoder::Done);

        return context.pos;
    }

    // FIX: A little kludgey, lets use reuse this exact DecoderSubjectBase over again for a dispatch
    // consider this only for test purposes
    void reset()
    {
        new (&decoder) Decoder;
    }
};
#endif

}}
