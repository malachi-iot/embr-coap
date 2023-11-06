#pragma once

#ifdef ESP_PLATFORM
#include <esp_log.h>
#endif

#include "sub.h"

// DEBT: It's possible to make this code c++11 compliant if we use
// explicit full class functors

namespace embr { namespace coap {
inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
template <ESTD_CPP_CONCEPT(concepts::StateFunctor) F, class ...Args>
inline void Subcontext<Substates...>::visit(F&& f, Args&&...args)
{
#if __cpp_generic_lambdas >= 201707L
    state_.visit_index([&]<estd::size_t I, concepts::State T>(estd::variadic::v2::instance<I, T> i)
    {
        f(i.value, std::forward<Args>(args)...);
        return true;
    });
#else
#warning Subcontext requires c++20
#endif
}


template <class T>
struct is_tuple : estd::false_type {};

template <class... Args>
struct is_tuple<estd::tuple<Args...>> : estd::true_type {};



template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, class F>
void Subcontext<Substates...>::create(int id_path, Context& context, F&& f)
{
#if __cpp_generic_lambdas >= 201707L
    // Emplace factory!
    state_.visit([&]<unsigned I, concepts::State T>(estd::variadic::v2::type<I, T> t)
    {
        if(id_path != T::id_path) return false;

        auto v = f(t);
        constexpr bool v2 = estd::is_same_v<decltype(v), nullptr_t>;

        //static_assert(v2, "At the moment only nullptr_t is supported");

        if constexpr(v2)
        {
            state_.template emplace<T>(context);
        }
        else if constexpr(is_tuple<T>::value)
        {
            estd::apply([&]<class ...Args>(Args const&... args)
            {
                state_.template emplace<T>(context, args...);
            }, v);
        }
        else
        {
            state_.template emplace<T>(context, std::move(v));
        }

        return true;
    });
#endif
}

template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
// DEBT: Use a concept here for 'Context'
template <class Context>
void Subcontext<Substates...>::on_uri_query(const embr::coap::event::option& e, Context& context)
{
    const query q = internal::v1::split(e);     // DEBT: Not *every* condition wants the key=value format for URIs, though most do
    const estd::string_view key = estd::get<0>(q);

    if(key.data() == nullptr)
    {
        // DEBT: Use fake ESP_LOGx or finish up the clog wrapper
#ifdef ESP_PLATFORM
        // NOTE: In our case we require key=value format.  But I believe URIs
        // in general are more free form.  So, this IS a malformed query, but
        // only in the context of this application - not a CoAP URI
        ESP_LOGI(TAG, "malformed query: %.*s", e.chunk.size(), e.string().data());
#endif
        return;
    }

    /* from_query already does this, but I think I might prefer it here
    const estd::string_view value = estd::get<1>(q);

    ESP_LOGV(TAG, "on_notify: uri-query key=%.*s value=%.*s",
        key.size(), key.data(),
        value.size(), value.data());    */

#if __cpp_generic_lambdas >= 201707L
    visit([&]<concepts::State S>(S& s) { s.on_option(q); });
#endif
}


template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
template <class Streambuf>
void Subcontext<Substates...>::on_payload(Streambuf& s)
{
#if __cpp_generic_lambdas >= 201707L
    estd::detail::basic_istream<Streambuf&> in(s);

    visit([&]<concepts::State S>(S &s) { s.on_payload(in); });
#endif
}


/// Returns whether or not subcontext picked up and processed request
/// Note that populating auto response_code does count as a pickup
template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
template <
    ESTD_CPP_CONCEPT(concepts::StreambufEncoder) Encoder,
    ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
bool Subcontext<Substates...>::on_completed(Encoder& encoder, Context& context)
{
    using C = embr::coap::Header::Code;

#if __cpp_generic_lambdas >= 201707L
    return state_.visit_index([&]<std::size_t I, concepts::State T>(estd::variadic::instance<I, T> i)
    {
        C code = i->response();

        if(code == C::Empty)
            return i->completed(encoder);
        else
            context.response_code = code;

        return true;
    }) != -1;
#else
    return false;
#endif
}

}}
}}
