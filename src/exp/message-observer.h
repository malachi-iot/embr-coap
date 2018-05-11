#pragma once

#include <mc/mem/platform.h>
#include <coap/experimental-observer.h>

#ifdef FEATURE_CPP_VARIADIC
// If we have variadic, then we assume we also have type traits
#include <type_traits>
#endif

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

    /*
    template <class TMessageObserver, bool has_constructor>
    TMessageObserver* _on_header_invoker_helper_1;

    template <class TMessageObserver>
              //typename = typename std::enable_if<std::is_constructible<TMessageObserver, context_t&>::value>::type>
    TMessageObserver* _on_header_invoker_helper_1<TMessageObserver, true>()
    {
        context_t& context = base_t::context();
        TMessageObserver* mo = new (context) TMessageObserver(context);
        return mo;
    }

    template <class TMessageObserver>
              //typename = typename std::enable_if<!std::is_constructible<TMessageObserver, context_t&>::value>::type>
    TMessageObserver* _on_header_invoker_helper_1()
    {
        context_t& context = base_t::context();
        TMessageObserver* mo = new (context) TMessageObserver;
        mo->context(context);
        return mo;
    }

    template <class TMessageObserver>
    TMessageObserver* _on_header_invoker_helper_2()
    {

    } */

    template <class TMessageObserver>
    void _on_header_invoker(const Header& header)
    {
        context_t& context = base_t::context();

        TMessageObserver* mo;// = _on_header_invoker_helper_1<TMessageObserver>();

        mo->on_header(header);

        mo->~TMessageObserver();
    }

    // ala https://stackoverflow.com/questions/7124969/recursive-variadic-template-to-print-out-the-contents-of-a-parameter-pack
    template <class TLast>
    void on_header_invoker(const Header& header)
    {
        _on_header_invoker<TLast>(header);
    }

    template <class TFirst, class TSecond, class... TArgs>
    void on_header_invoker(const Header& header)
    {
        _on_header_invoker<TFirst>(header);
        on_header_invoker<TSecond, TArgs...>(header);
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
