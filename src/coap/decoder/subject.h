/**
 * @file
 * @author  Malachi Burke
 */

#pragma once

#include "../decoder.h"
#include "../../coap-dispatcher.h"

#include <estd/span.h>
#include "../../exp/events.h"

namespace moducom { namespace coap {

namespace experimental {


// TODO: If we like these observer helpers, move them into (estd) observer.h not subject.h
template <class TNotifier1, class TNotifier2 = void, class TNotifier3 = void, class TNotifier4 = void, class TNotifier5 = void, class TDecider = void>
struct observer_base_base;

template <class TNotifier1>
struct observer_base_base<TNotifier1>
{
    virtual void on_notify(const TNotifier1&) = 0;
};

template <class TNotifier1, class TNotifier2, class TNotifier3, class TNotifier4, class TNotifier5>
struct observer_base_base<TNotifier1, TNotifier2, TNotifier3, TNotifier4, TNotifier5, int>
{
    virtual void on_notify(const TNotifier1&) = 0;
    virtual void on_notify(const TNotifier2&) = 0;
    virtual void on_notify(const TNotifier3&) = 0;
    virtual void on_notify(const TNotifier4&) = 0;
    virtual void on_notify(const TNotifier5&) = 0;
};

// super-experimental
template <class TObserver, class TNotifier1, class TNotifier2 = void, class TNotifier3 = void, class TNotifier4 = void, class TNotifier5 = void, class TDecider = void>
struct observer_base_base_wrapper;

template <class TObserver, class TNotifier1, class TNotifier2, class TNotifier3, class TNotifier4, class TNotifier5>
struct observer_base_base_wrapper<TObserver, TNotifier1, TNotifier2, TNotifier3, TNotifier4, TNotifier5, int> :
        observer_base_base<TNotifier1, TNotifier2, TNotifier3, TNotifier4, TNotifier5, int>
{
    TObserver observer;

    virtual void on_notify(const TNotifier1& e) OVERRIDE
    {
        observer.on_notify(e);
    }

    virtual void on_notify(const TNotifier2& e) OVERRIDE
    {
        observer.on_notify(e);
    }

    virtual void on_notify(const TNotifier3& e) OVERRIDE
    {
        observer.on_notify(e);
    }

    virtual void on_notify(const TNotifier4& e) OVERRIDE
    {
        observer.on_notify(e);
    }

    virtual void on_notify(const TNotifier5& e) OVERRIDE
    {
        observer.on_notify(e);
    }
};

template <class TObserver, class TNotifier1>
struct observer_base_base_wrapper<TObserver, TNotifier1> :
        observer_base_base<TNotifier1>
{
    TObserver observer;

    virtual void on_notify(const TNotifier1& e) OVERRIDE
    {
        observer.on_notify(e);
    }
};


// FIX: change name, only needed if using virtualized subject/observer
struct observer_base
{
    virtual void on_notify(const header_event&) = 0;
    virtual void on_notify(const token_event&) = 0;
    virtual void on_notify(const option_event&) = 0;
    virtual void on_notify(const payload_event&) = 0;
    virtual void on_notify(completed_event) = 0;
};

// For having non-virtualized observers participate in virtualized environment
template <class TObserver>
struct observer_wrapper : observer_base
{

};

// TSubject = one of the new embr::*::subject classes
// NOTE: Not perfect location, a global function, but not terrible either.  Perhaps
// attaching it to Decoder, since TSubject is entirely ephemeral anyway?
// Also be advised this actually does a process iterate as well as a notify
template <class TSubject, class TContext>
bool decode_and_notify(TSubject& subject,
                         Decoder& decoder,
                         Decoder::Context& context,
                         TContext& app_context);


template <class TSubject>
bool notify_from_decoder(TSubject& subject, Decoder& decoder, Decoder::Context& context)
{
    int fake_app_context = 0;

    return decode_and_notify(subject, decoder, context, fake_app_context);
}


template <class TSubject>
bool decode_and_notify(TSubject& subject, Decoder& decoder, Decoder::Context& context)
{
    int fake_app_context;

    return decode_and_notify(subject, decoder, context, fake_app_context);
}

}

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
    bool dispatch_iterate(Decoder::Context& context);

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

        while(!dispatch_iterate(context) && decoder.state() != Decoder::Done);

        return context.pos;
    }

    // FIX: A little kludgey, lets use reuse this exact DecoderSubjectBase over again for a dispatch
    // consider this only for test purposes
    void reset()
    {
        new (&decoder) Decoder;
    }
};

}}
