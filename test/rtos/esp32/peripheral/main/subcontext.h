#pragma once

#include <coap/platform/lwip/context.h>

// TODO: Move all this into embr::coap::v1 namespace

struct CoapSubcontextBase
{
    using query = estd::pair<estd::string_view, estd::string_view>;

    static constexpr const char* TAG = "CoapSubcontext";
};


CoapSubcontextBase::query split(const embr::coap::event::option& e);

// Consider sequestering this into a 'concepts' namespace since pascal case everywhere
// may lead to collisions
template <class T>
concept Subtate = requires(T s)
{
    //using query = estd::pair<estd::string_view, estd::string_view>;
    s.on_option(estd::pair<estd::string_view, estd::string_view>{});
    []<class Payload>(Payload& p) { std::declval<T>().on_payload(p); };
    []<class Encoder>(Encoder& e) { std::declval<T>().completed(e); };
    //s.id_path;
    { T::id_path } -> std::convertible_to<int>;
};

// Looking into https://stackoverflow.com/questions/64694218/how-to-express-concepts-over-variadic-template
// But this isn't quite there
//template <class ...Args>
//concept Substates = (Substate(Args) && ...)


template <Subtate ...Substates>
class CoapSubcontext : CoapSubcontextBase
{
    estd::variant<Substates...> state_;

    template <class F, class ...Args>
    void visit(F&& f, Args&&...args);

public:
    estd::variant<Substates...>& state() { return state_; }

    template <class Context>
    void create(int id_path, Context& context);

    template <class Context>
    void on_uri_query(const embr::coap::event::option&, Context& context);

    template <class Streambuf>
    void on_payload(Streambuf&);

    template <class Encoder, class Context>
    bool on_completed(Encoder&, Context&);
};


template <Subtate ...Substates>
template <class F, class ...Args>
inline void CoapSubcontext<Substates...>::visit(F&& f, Args&&...args)
{
    state_.visit_index([&]<estd::size_t I, Subtate T>(estd::variadic::v2::instance<I, T> i)
    {
        f(i.value, std::forward<Args>(args)...);
        return true;
    });
}


template <Subtate ...Substates>
template <class Context>
void CoapSubcontext<Substates...>::create(int id_path, Context& context)
{
    // Emplace factory!
    state_.visit([&]<unsigned I, Subtate T>(estd::variadic::v2::type<I, T>)
    {
        if(id_path != T::id_path) return false;

        state_.template emplace<T>(context);
        return true;
    });
}

template <Subtate ...Substates>
template <class Context>
void CoapSubcontext<Substates...>::on_uri_query(const embr::coap::event::option& e, Context& context)
{
    const query q = split(e);
    const estd::string_view key = estd::get<0>(q);
    
    if(key.data() == nullptr)
    {
        // NOTE: In our case we require key=value format.  But I believe URIs
        // in general are more free form.  So, this IS a malformed query, but
        // only in the context of this application - not a CoAP URI
        ESP_LOGI(TAG, "malformed query: %.*s", e.chunk.size(), e.string().data());
        return;
    }

    /* from_query already does this, but I think I might prefer it here
    const estd::string_view value = estd::get<1>(q);

    ESP_LOGV(TAG, "on_notify: uri-query key=%.*s value=%.*s",
        key.size(), key.data(),
        value.size(), value.data());    */

    visit([&]<Subtate S>(S& s) { s.on_option(q); });
}


template <Subtate ...Substates>
template <class Streambuf>
void CoapSubcontext<Substates...>::on_payload(Streambuf& s)
{
    estd::detail::basic_istream<Streambuf&> in(s);

    visit([&]<Subtate S>(S &s) { s.on_payload(in); });
}


/// Returns whether or not subcontext picked up and processed request
/// Note that populating auto response_code does count as a pickup
template <Subtate ...Substates>
template <class Encoder, class Context>
bool CoapSubcontext<Substates...>::on_completed(Encoder& encoder, Context& context)
{
    using C = embr::coap::Header::Code;

    return state_.visit_index([&]<unsigned I, Subtate T>(estd::variadic::instance<I, T> i)
    {
        C code = i->response();

        if(code == C::Empty)
            return i->completed(encoder);
        else
            context.response_code = code;

        return true;
    }) != -1;
}