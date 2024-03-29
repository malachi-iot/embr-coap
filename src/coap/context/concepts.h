#pragma once

#include <estd/string_view.h>

#include "tags.h"

#if __cpp_concepts && __cpp_lib_concepts
// DEBT: Use __cpp_lib_concepts or whatever its name is
#include <concepts>

#include "../encoder/concepts.h"

namespace embr { namespace coap { namespace concepts {

inline namespace context { inline namespace v1 {

// FIX: Not working right
//template <class T>
//concept IncomingContext = std::derived_from<T, embr::coap::tags::incoming_context>;

template <class T>
concept IncomingContext = requires(T c)
{
    // For some reason LwipIncomingContext doesn't play nice with this
    //c.token();
    //{ c.header() } -> std::convertible_to<Header>;

    typename T::istreambuf_type;
};

template <class T>
concept OutgoingContext = requires(T c)
{
    typename T::ostreambuf_type;
    typename T::encoder_type;
};


template <class T>
concept ReplyContext = IncomingContext<T> && OutgoingContext<T> && requires(T c)
{
    // "ExtraContext" area, which is fine functionality just a dubious name.  Not 100%
    // sure I want every ReplyContext to have to conform to that though
    c.response_code;


    c.reply(std::declval<typename T::encoder_type&>());
};


}}

inline namespace subcontext { inline namespace v1 {


// DEBT: Move this out elsewhere, though I find it hard to believe 'concepts'
// library doesn't already have this.  It does have 'invocable' but that doesn't
// check return type
template <class F, typename R = void, class ...Args>
concept Functor = requires(F f, Args&&... args)
{
    { f(args...) } -> std::convertible_to<R>;
};



// DEBT: We'd like this to be named 'Subcontext', however that name
// collides at the moment with its container
template <class T>
concept State = requires(T s)
{
    //using query = estd::pair<estd::string_view, estd::string_view>;
    s.on_option(estd::pair<estd::string_view, estd::string_view>{});
    { s.response() } -> std::convertible_to<Header::Code>;
    []<class Streambuf>(estd::detail::basic_istream<Streambuf>& p) { std::declval<T>().on_payload(p); };
    //[]<class Payload>(Payload& p) { std::declval<T>().on_payload(p); };
    []<StreambufEncoder Encoder>(Encoder& e) { std::declval<T>().completed(e); };
    //s.id_path;
    { T::id_path } -> std::convertible_to<int>;
    []<IncomingContext Context>(Context& c) { T ss(c); };
};

template <class F>
concept StateFunctor = requires(F f)
{
    []<State S, class... Args>(S& s, Args&&...args)
    { std::declval<F>(s, args...); };
};


}}

}}}
#endif
