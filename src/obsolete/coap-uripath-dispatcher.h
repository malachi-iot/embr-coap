//
// Created by malachi on 2/14/18.
//
#pragma  once

#include <obsolete/coap-dispatcher.h>
#include "mc/experimental-factory.h"
//#include "mc/objstack.h"

namespace embr { namespace coap {

// Specialized prefix checker which does not expect s to be
// null terminated, but does expect prefix to be null terminated
template <typename TChar>
inline bool starts_with(const TChar* s, int slen, const char* prefix)
{
    while(*prefix && slen--)
    {
        if(*prefix++ != *s++) return false;
    }

    return true;
}


inline bool starts_with(pipeline::MemoryChunk::readonly_t chunk, const char* prefix)
{
    return starts_with(chunk.data(), chunk.length(), prefix);
}


// though in theory this adds another chain to the vtable list, I guess it being
// so templatized it doesn't actually register as such
template <bool allow_response = false>
class UriPathDispatcherHandlerBaseBase :
        public DecoderObserverBase<ObserverContext>
{
protected:
    const char* prefix;

    UriPathDispatcherHandlerBaseBase(const char* prefix) : prefix(prefix)
    {}

    void on_header(Header header) OVERRIDE
    {
        if(!allow_response)
            if(!header.is_request()) interested(Never);
    }
};

// a 1:1 uripath to observer mapper
template <class TObserver, bool destruct_observer = false>
class UriPathDispatcherHandlerBase : public UriPathDispatcherHandlerBaseBase<>
{
protected:
    typedef UriPathDispatcherHandlerBaseBase base_t;
    TObserver& _observer;

    TObserver& observer() const { return _observer; }

public:
    UriPathDispatcherHandlerBase(const char* prefix, TObserver& observer)
            : base_t(prefix),
              _observer(observer)
    {

    }

    void on_option(number_t number, uint16_t length) OVERRIDE
    {
        if(is_always_interested())
        {
            observer().on_option(number, length);
            return;
        }
    }

    void on_option(number_t number,
                   const pipeline::MemoryChunk::readonly_t& option_value_part,
                   bool last_chunk) OVERRIDE
    {
        // If we're always interested, then we've gone into pass thru mode
        if(is_always_interested())
        {
            observer().on_option(number, option_value_part, last_chunk);
            return;
        }

        switch(number)
        {
            case Option::UriPath:
                // FIX: I think we don't need this, because path is already
                // chunked out so 99% of the time we'll want to compare the
                // whole string not just the prefix - but I suppose comparing
                // the as a prefix doesn't hurt us
                if(starts_with(option_value_part, prefix))
                    interested(Always);
                else
                    // If we don't get what we want right away, then we are
                    // no longer interested (more prefix style behavior)
                    interested(Never);

                // TODO: We'll want to not only indicate interest but also
                // have a subordinate dispatcher to act on further observations
                break;

            // I'd leave this out, but a lot of compilers don't like ignoring
            // case statements
            default: break;
        }
    }

    void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part, bool last_chunk) OVERRIDE
    {
        // Currently interested can happen if we never encounter the general uri-path
        // we're looking for (currently gets changed to always or never during URI processing)
        if(interested() == Currently) return;

        ASSERT_WARN(true, is_always_interested(), "Should only arrive here if interested");

        observer().on_payload(payload_part, last_chunk);
    }

    /*
    ~UriPathDispatcherHandlerBase()
    {
        if(destruct_observer)
            observer.~IDispatcherHandler();
    } */
};


// links a single observer to a particular uri prefix

template <class TRequestContext>
class SingleUriPathObserver :
        public UriPathDispatcherHandlerBase<IDecoderObserver<TRequestContext> >
{
    typedef UriPathDispatcherHandlerBase<IDecoderObserver<TRequestContext> > base_t;
public:
    SingleUriPathObserver(const char* prefix, IDecoderObserver<TRequestContext>& observer)
            : base_t(prefix, observer)
    {

    }
};


template <const char* uri_path, experimental::dispatcher_handler_factory_fn* factories, int count>
IDecoderObserver<ObserverContext>*
        uri_plus_factory_dispatcher(ObserverContext& ctx)
{
    // FIX: Clumsy, but should be effective for now; ensures order of allocation is correct
    //      so that later deallocation for objstack doesn't botch
    void* buffer1 = ctx.objstack.alloc(sizeof(SingleUriPathObserver<ObserverContext>));

    experimental::FactoryDispatcherHandler<ObserverContext>* fdh =
            new (ctx) experimental::FactoryDispatcherHandler<ObserverContext>(
                    ctx,
                    factories, count);

    return new (buffer1) SingleUriPathObserver<ObserverContext> (uri_path, *fdh);
}


// Creates a unique static TMessageObserver associated with this uri_path
template <const char* uri_path, class TMessageObserver>
IDecoderObserver<ObserverContext>* uri_plus_observer_dispatcher(ObserverContext& ctx)
{
    static TMessageObserver observer;

    // FIX: kludgey, and though SFINAE would be helpful, would probably
    // not alleviate the kludginess enough.  reserved-dispatcher likely
    // a better solution so that we can push context thru via constructor
    observer.context(ctx);

    return new (ctx) SingleUriPathObserver<ObserverContext>(uri_path, observer);
}



namespace experimental {

// Newer, better replacement UriDispatcherHandler
// does not maintain interested-state , since this particular
// dispatcher chooses one at most interested URI.  Because of that,
// objstack usage is potentially a small waste - but we'll use it
// here anyway as a proving grounds of its usefulness
//
// an aggregate of single-uri-path-element to IDispatcherHandler* mappings
template <class TRequestContext = ObserverContext>
class AggregateUriPathObserver : public DecoderObserverBase<TRequestContext>
{
    typedef DecoderObserverBase<TRequestContext> base_t;
    typedef typename base_t::context_t request_context_t;

public:
    struct Context
    {
    public:
        request_context_t& context;

        Context(request_context_t& context) :
            context(context) {}

        Context(const Context& copy_from) :
            context(copy_from.context) {}
    };

    // TODO: This is where revamped estd::map version will live
    typedef IDecoderObserver<request_context_t> decoder_observer_t;
    typedef FnFactoryTraits<const char*, decoder_observer_t*, Context&> traits_t;
    typedef FnFactoryHelper<traits_t> fn_t;
    typedef typename fn_t::factory_t factory_t;
    typedef typename fn_t::item_t item_t;
    typedef typename base_t::number_t number_t;

protected:

    factory_t factory;

    Context context;

    decoder_observer_t* handler;

public:

    // NOTE: fanciness not really necessary just a generic class T
    // would probably be fine, factory itself dissects all that
    template <const size_t N>
    AggregateUriPathObserver(request_context_t& incoming_context,
                         item_t (&items) [N]) :
        factory(items),
        context(incoming_context),
        handler(NULLPTR)
    {
    }

    AggregateUriPathObserver(ObserverContext& incoming_context,
                         item_t* items, size_t item_count) :
            factory(items, item_count),
            context(incoming_context),
            handler(NULLPTR)
    {
    }


    template <const size_t N>
    AggregateUriPathObserver(Context& context,
                         item_t (&items) [N]) :
        factory(items),
        context(context),
        handler(NULLPTR)
    {
    }

    virtual void on_option(number_t number,
                           const pipeline::MemoryChunk::readonly_t& option_value_part,
                           bool last_chunk) OVERRIDE;

    virtual void on_payload(const pipeline::MemoryChunk::readonly_t& payload_part,
                            bool last_chunk) OVERRIDE;

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE;
#else
    // TODO: Issue a warning or error that on_complete is needed
#endif
};

}

}}

#include "coap-uripath-dispatcher.hpp"
