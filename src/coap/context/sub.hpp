#pragma once

#ifdef ESP_PLATFORM
#include <esp_log.h>
#endif

#include "sub.h"

// DEBT: It's possible to make this code c++11 compliant if we use
// explicit full class functors

namespace embr { namespace coap {
inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::Substate)... Substates>
template <class F, class ...Args>
inline void CoapSubcontext<Substates...>::visit(F&& f, Args&&...args)
{
    state_.visit_index([&]<estd::size_t I, concepts::Substate T>(estd::variadic::v2::instance<I, T> i)
    {
        f(i.value, std::forward<Args>(args)...);
        return true;
    });
}


template <ESTD_CPP_CONCEPT(concepts::Substate)... Substates>
template <class Context>
void CoapSubcontext<Substates...>::create(int id_path, Context& context)
{
    // Emplace factory!
    state_.visit([&]<unsigned I, concepts::Substate T>(estd::variadic::v2::type<I, T>)
    {
        if(id_path != T::id_path) return false;

        state_.template emplace<T>(context);
        return true;
    });
}

template <ESTD_CPP_CONCEPT(concepts::Substate)... Substates>
// DEBT: Use a concept here for 'Context'
template <class Context>
void CoapSubcontext<Substates...>::on_uri_query(const embr::coap::event::option& e, Context& context)
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

    visit([&]<concepts::Substate S>(S& s) { s.on_option(q); });
}


template <ESTD_CPP_CONCEPT(concepts::Substate)... Substates>
template <class Streambuf>
void CoapSubcontext<Substates...>::on_payload(Streambuf& s)
{
    estd::detail::basic_istream<Streambuf&> in(s);

    visit([&]<concepts::Substate S>(S &s) { s.on_payload(in); });
}


/// Returns whether or not subcontext picked up and processed request
/// Note that populating auto response_code does count as a pickup
template <ESTD_CPP_CONCEPT(concepts::Substate)... Substates>
template <
    ESTD_CPP_CONCEPT(concepts::StreambufEncoder) Encoder,
    ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
bool CoapSubcontext<Substates...>::on_completed(Encoder& encoder, Context& context)
{
    using C = embr::coap::Header::Code;

    return state_.visit_index([&]<std::size_t I, concepts::Substate T>(estd::variadic::instance<I, T> i)
    {
        C code = i->response();

        if(code == C::Empty)
            return i->completed(encoder);
        else
            context.response_code = code;

        return true;
    }) != -1;
}

}}
}}
