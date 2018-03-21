//
// Created by malachi on 2/14/18.
//
#pragma  once

#include "coap-dispatcher.h"
#include "mc/experimental-factory.h"
#include "mc/objstack.h"

namespace moducom { namespace coap {

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
class UriPathDispatcherHandlerBaseBase : public experimental::DispatcherHandlerBase
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


class UriPathDispatcherHandler :
        public UriPathDispatcherHandlerBase<experimental::IDispatcherHandler>
{
    typedef UriPathDispatcherHandlerBase<experimental::IDispatcherHandler> base_t;
public:
    UriPathDispatcherHandler(const char* prefix, experimental::IDispatcherHandler& observer)
            : base_t(prefix, observer)
    {

    }
};


template <const char* uri_path, experimental::dispatcher_handler_factory_fn* factories, int count>
experimental::IDispatcherHandler* uri_plus_factory_dispatcher(experimental::FactoryDispatcherHandlerContext& ctx)
{
    pipeline::MemoryChunk& chunk = ctx.handler_memory;
    pipeline::MemoryChunk& uri_handler_chunk = chunk;
    // semi-objstack behavior
    CONSTEXPR size_t sizeUriPathDispatcher = sizeof(UriPathDispatcherHandler);
    CONSTEXPR size_t sizeFactoryDispatcher = sizeof(experimental::FactoryDispatcherHandler);

    pipeline::MemoryChunk sub_handler_chunk = chunk.remainder(sizeUriPathDispatcher);
    pipeline::MemoryChunk sub_handler_inner_chunk =
            sub_handler_chunk.remainder(sizeFactoryDispatcher);

    experimental::FactoryDispatcherHandler* fdh =
            new (sub_handler_chunk.data()) experimental::FactoryDispatcherHandler(
            sub_handler_inner_chunk,
            ctx.incoming_context,
            factories, count);

    return new (uri_handler_chunk.data()) UriPathDispatcherHandler(uri_path, *fdh);
}


// Creates a unique static TMessageObserver associated with this uri_path
template <const char* uri_path, class TMessageObserver>
experimental::IDispatcherHandler* uri_plus_observer_dispatcher(experimental::FactoryDispatcherHandlerContext& ctx)
{
    static TMessageObserver observer;

    // FIX: kludgey, and though SFINAE would be helpful, would probably
    // not alleviate the kludginess enough.  reserved-dispatcher likely
    // a better solution so that we can push context thru via constructor
    observer.set_context(ctx.incoming_context);

    return new (ctx.handler_memory.data()) UriPathDispatcherHandler(uri_path, observer);
}



namespace experimental {

// Newer, better replacement UriDispatcherHandler
// does not maintain interested-state , since this particular
// dispatcher chooses one at most interested URI.  Because of that,
// objstack usage is potentially a small waste - but we'll use it
// here anyway as a proving grounds of its usefulness
class UriDispatcherHandler : public experimental::DispatcherHandlerBase
{
    typedef experimental::DispatcherHandlerBase base_t;

public:
    struct Context
    {
        dynamic::ObjStack objstack;
        IncomingContext& context;

        Context(const dynamic::ObjStack& objstack,
                IncomingContext& context) :
            objstack(objstack),
            context(context) {}
    };

    typedef FnFactoryTraits<const char*, IDispatcherHandler*, Context&> traits_t;
    typedef FnFactoryHelper<traits_t> fn_t;
    typedef fn_t::factory_t factory_t;
    typedef fn_t::item_t item_t;

protected:

    factory_t factory;

    Context context;

    IDispatcherHandler* handler;

public:

    // NOTE: fanciness not really necessary just a generic class T
    // would probably be fine, factory itself dissects all that
    template <const size_t N>
    UriDispatcherHandler(pipeline::MemoryChunk chunk,
                         IncomingContext& incomingContext,
                         fn_t::item_t (&items) [N]) :
        factory(items),
        context(chunk, incomingContext),
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
