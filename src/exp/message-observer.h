#pragma once

#include <mc/mem/platform.h>
#include <coap/experimental-observer.h>

#ifdef FEATURE_CPP_VARIADIC
// If we have variadic, then we assume we also have type traits
#include <type_traits>
#include <estd/type_traits.h> // for estd::enable_if_t, otherwise it's only present in c++14
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

    // for message observers with constructors
    // thanks to https://stackoverflow.com/questions/32167213/select-constructor-through-sfinae-in-template-arguments
    template <class TMessageObserver,
              estd::enable_if_t<
                  std::is_constructible<TMessageObserver, context_t&>::value
                  >* = nullptr
              >
    TMessageObserver* ctor_selector()
    {
        context_t& context = base_t::context();
        return new (context) TMessageObserver(context);
    }

    // for message observers without constructors
    template <class TMessageObserver,
              estd::enable_if_t<
                  !std::is_constructible<TMessageObserver, context_t&>::value
                  >* = nullptr
              >
    TMessageObserver* ctor_selector()
    {
        context_t& context = base_t::context();
        TMessageObserver* mo = new (context) TMessageObserver;
        mo->context(context);
        return mo;
    }

    template <class TMessageObserver>
    void _on_header_invoker(const Header& header)
    {
        context_t& context = base_t::context();

        TMessageObserver* mo = ctor_selector<TMessageObserver>();// = _on_header_invoker_helper_1<TMessageObserver>();

        mo->on_header(header);

        mo->~TMessageObserver();
    }

    // TODO: Move this to PGGCC-10
    // TParam a little cheezy, but because multiple variadics are tricky, we'll do it for now
    template <template <class> class TFunc, class TParam, class TLast, class... TArgs>
    void pack_function_helper(TFunc<TLast> func, TParam param1)
    {
        func(param1);
    }

    template <template <class> class TFunc, class TParam, class TFirst, class TSecond, class... TArgs>
    void pack_function_helper(TFunc<TFirst> func, TParam param1)
    {
        func(param1);
        pack_function_helper<TFunc, TParam, TSecond, TArgs...>(func, param1);
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
