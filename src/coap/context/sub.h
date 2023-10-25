#pragma once

#include <estd/variant.h>

#include "concepts.h"
#include "../encoder/concepts.h"

#include "../token.h"
#include "../header.h"
#include "../decoder/events.h"

namespace embr { namespace coap {

// 'internal' namespace used all over the place and gets into
// ambiguity if we put it under an inline namespace, so handling
// versioning distinctly for it
namespace internal { inline namespace v1 {

struct CoapSubcontextBase
{
    using query = estd::pair<estd::string_view, estd::string_view>;

    static constexpr const char* TAG = "CoapSubcontext";

    struct undefined
    {
        using code_type = embr::coap::Header::Code;

        static bool constexpr on_option(const query&) { return {}; }

        template <class Streambuf>
        static bool constexpr on_payload(Streambuf&) { return {}; }

        static constexpr code_type response()
        {
            return code_type::Empty;
        }

        template <class Encoder>
        static constexpr bool completed(Encoder&)
        {
            return {};
        }
    };

    struct unknown : undefined
    {
        static constexpr int id_path = -1;

        unknown() = default;

        template <class Context>
        constexpr explicit unknown(Context&) {}   // dummy, just for factory to be satisfied
    };

    template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
    struct base : undefined
    {
        Context& context;

        const embr::coap::Header& header() const
        {
            return context.header();
        }

        /*
        void build_reply(encoder_type& e, code_type c)
        {
            embr::coap::build_reply(context, encoder, code);
        }   */

        constexpr base(Context& c) : context{c} {}
    };
};

// DEBT: This feels clumsy floating around here, even though it's
// internal
embr::coap::internal::v1::CoapSubcontextBase::query split(const embr::coap::event::option& e);

}}

inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
class Subcontext : internal::v1::CoapSubcontextBase
{
    // NOTE: This is likely a better job for variant_storage, since we know based on URI which particular
    // state we're interested in and additionally we'd prefer not to initialize *any* - so in other words
    // somewhere between a union and a variant, which is what variant_storage really is
    using state_type = estd::variant<unknown, Substates...>;

    state_type state_;

    template <class F, class ...Args>
    void visit(F&& f, Args&&...args);

public:
    state_type& state() { return state_; }

    template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
    void create(int id_path, Context& context);

    template <class Context>
    void on_uri_query(const embr::coap::event::option&, Context& context);

    template <class Streambuf>
    void on_payload(Streambuf&);

    // TODO: Look for 'ExtraContext' in all its terrible name glory here as part of
    // concept
    template <
        ESTD_CPP_CONCEPT(concepts::StreambufEncoder) Encoder,
        ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
    bool on_completed(Encoder&, Context&);
};


}}
}}
