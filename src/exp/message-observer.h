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
// keeping with experimental naming because we haven't resolved naming for
// virtualized-capable vs template-only.  Right now it's implicitly
// 'DecoderObserver' vs 'MessageObserver' but that's inconsistent and unclear
// IDEA: Consider naming the template-only stuff the same, but stuffing it into 'layer1'
// note that since this is embedded-oriented, hiding the best-candidate stuff in layer1
// might not be our best choice.
template <class TIncomingContext = ObserverContext, class ...TMessageObservers>
class AggregateMessageObserver : public MessageObserverBase<TIncomingContext>
{
    typedef MessageObserverBase<TIncomingContext> base_t;
    typedef internal::option_number_t option_number_t;
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
    void dtor_helper(TMessageObserver* mo)
    {
        // TODO: Use this because sometimes we won't be calling dtor or deallocating
        // (for 'reserved' class observers)
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
    template <class TMessageObserver> // TMessageObserver aka TLast
    void on_header_invoker(const Header& header)
    {
        TMessageObserver* mo = ctor_selector<TMessageObserver>();// = _on_header_invoker_helper_1<TMessageObserver>();

        mo->on_header(header);

        dtor_helper(mo);
    }

    template <class TMessageObserver>
    void on_token_invoker(const ro_chunk_t& token, bool last_chunk)
    {
        TMessageObserver* mo = ctor_selector<TMessageObserver>();// = _on_header_invoker_helper_1<TMessageObserver>();

        mo->on_token(token, last_chunk);

        dtor_helper(mo);
    }

    template <class TMessageObserver>
    void on_option_invoker(option_number_t number,
                   const ro_chunk_t& option_value_part,
                   bool last_chunk)
    {
        TMessageObserver* mo = ctor_selector<TMessageObserver>();// = _on_header_invoker_helper_1<TMessageObserver>();

        mo->on_option(number, option_value_part, last_chunk);

        dtor_helper(mo);
    }

    template <class TMessageObserver>
    void on_option_invoker(option_number_t number, uint16_t length)
    {
        TMessageObserver* mo = ctor_selector<TMessageObserver>();

        mo->on_option(number, length);

        dtor_helper(mo);
    }

    template <class TFirst, class TSecond, class... TArgs>
    void on_header_invoker(const Header& header)
    {
        on_header_invoker<TFirst>(header);
        on_header_invoker<TSecond, TArgs...>(header);
    }

    template <class TFirst, class TSecond, class... TArgs>
    void on_token_invoker(const ro_chunk_t& token, bool last_chunk)
    {
        on_token_invoker<TFirst>(token, last_chunk);
        on_token_invoker<TSecond, TArgs...>(token, last_chunk);
    }


    template <class TFirst, class TSecond, class... TArgs>
    void on_option_invoker(option_number_t number,
                   const ro_chunk_t& option_value_part,
                   bool last_chunk)
    {
        on_option_invoker<TFirst>(number, option_value_part, last_chunk);
        on_option_invoker<TSecond, TArgs...>(number, option_value_part, last_chunk);
    }

    template <class TFirst, class TSecond, class... TArgs>
    void on_option_invoker(option_number_t number, uint16_t length)
    {
        on_option_invoker<TFirst>(number, length);
        on_option_invoker<TSecond, TArgs...>(number, length);
    }

public:
    AggregateMessageObserver(context_t& c) : base_t(c) {}

    void on_header(const Header& header)
    {
        on_header_invoker<TMessageObservers...>(header);
    }

    void on_token(const ro_chunk_t& token, bool last_chunk = true)
    {
        on_token_invoker<TMessageObservers...>(token, last_chunk);
    }

    void on_option(option_number_t number, uint16_t length)
    {
        on_option_invoker<TMessageObservers...>(number, length);
    }

    void on_option(option_number_t number, const ro_chunk_t &chunk, bool last_chunk = true)
    {
        on_option_invoker<TMessageObservers...>(number, chunk, last_chunk);
    }
};
#endif

}}}
