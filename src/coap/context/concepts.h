#pragma once

#include <estd/string_view.h>

#include "tags.h"

#if __cpp_concepts
// DEBT: Use __cpp_lib_concepts or whatever its name is
#include <concepts>

namespace embr { namespace coap { namespace concepts {

inline namespace context { inline namespace v1 {

// FIX: Not working right
//template <class T>
//concept IncomingContext = std::derived_from<T, embr::coap::tags::incoming_context>;

template <class T>
concept IncomingContext = requires(T c)
{
    typename T::istreambuf_type;
};

template <class T>
concept OutgoingContext = requires(T c)
{
    //typename T::ostreambuf_type;
    typename T::encoder_type;
};


template <class T>
concept ReplyContext = IncomingContext<T> && OutgoingContext<T> && requires(T c)
{
    c.reply(std::declval<typename T::encoder_type&>());
};


}}

inline namespace subcontext { inline namespace v1 {

// DEBT: We'd like this to be named 'Subcontext', however that name
// collides at the moment with its container
template <class T>
concept Substate = requires(T s)
{
    //using query = estd::pair<estd::string_view, estd::string_view>;
    s.on_option(estd::pair<estd::string_view, estd::string_view>{});
    s.response();
    []<class Payload>(Payload& p) { std::declval<T>().on_payload(p); };
    []<class Encoder>(Encoder& e) { std::declval<T>().completed(e); };
    //s.id_path;
    { T::id_path } -> std::convertible_to<int>;
};

}}

}}}
#endif
