#pragma once

#include <mc/mem/platform.h>
#include <coap/experimental-observer.h>

namespace moducom { namespace coap { namespace experimental {

#ifdef FEATURE_CPP_VARIADIC
// templatized aggregation of messages using parameter pack
template <class TIncomingContext = ObserverContext, class ...TMessageObservers>
class AggregateMessageObserver : public MessageObserverBase<TIncomingContext>
{
    typedef MessageObserverBase<TIncomingContext> base_t;
    typedef experimental::option_number_t option_number_t;
    typedef pipeline::MemoryChunk::readonly_t ro_chunk_t;
    typedef typename base_t::context_t context_t;

    template <class TMessageObserver, class... TArgs>
    void on_header_invoker(const Header& header)
    {
        context_t& context = base_t::context();

        TMessageObserver* mo = new (context) TMessageObserver(context);

        mo->on_header(header);

        mo->~TMessageObserver();

        on_header_invoker<TArgs...>(header);
    }

public:
    AggregateMessageObserver(context_t& c) : base_t(c) {}

    void on_header(const Header& header)
    {
        on_header_invoker<TMessageObservers...>(header);
    }

};
#endif

}}}
