#pragma once

#include <estd/variant.h>

#include <embr/observer.h>

#ifdef ESP_PLATFORM
#include <esp_log.h>
#endif

#include "concepts.h"
#include "../encoder/concepts.h"

#include "../token.h"
#include "../header.h"
#include "../decoder/events.h"

#include "from_query.h"

// DEBT: Either filter out by c++20 or backport lambda behaviors below
// for pre c++20 operation

namespace embr { namespace coap {

namespace experimental {

// Pipeline would be a combo of Context and Observer
// probably tracked as a visited tuple within a context.  So, same
// generally as 'Observer' chain but those gently imply stateless,
// where this implies stateful.  
template <class T, T nullvalue>
class UriValuePipeline
{
    static constexpr const char* TAG = "UriValuePipeline";

protected:
    estd::layer1::optional<T, nullvalue> uri_value;
   

    bool populate_uri_value(const event::option& e)
    {
        // DEBT: As is the case all over, 'string' is assumed to be complete
        // data here

        bool success = coap::internal::from_string(e.string(), *uri_value).ec == 0;

#ifdef ESP_PLATFORM
        if(success)
            ESP_LOGV(TAG, "Found uri int=%d", *uri_value);
        else
            ESP_LOGD(TAG, "Was expecting uri int, but found none");
#endif

        return success;
    }

public:
    // DEBT: Use concept here
    template <class Context>
    void on_notify(const event::option& e, const Context& context)
    {
        const int node = context.found_node();
    }
};


template <class ...Args>
using Pipeline = embr::layer1::subject<Args...>;

struct PipelineObserver
{
    template <class Event, class Context>
    static void on_notify(const Event& e, Context& context)
    {
        context.pipeline.notify(e);
    }
};

}

// 'internal' namespace used all over the place and gets into
// ambiguity if we put it under an inline namespace, so handling
// versioning distinctly for it
namespace internal { inline namespace v1 {

struct SubcontextBase
{
    static constexpr const char* TAG = "CoapSubcontext";

    struct undefined
    {
        using query = v1::query;
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

    using query = undefined::query;

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

        constexpr explicit base(Context& c) : context{c} {}
    };
};

// DEBT: This feels clumsy floating around here, even though it's
// internal
embr::coap::internal::v1::SubcontextBase::query split(const embr::coap::event::option& e);

}}

inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::State)... Substates>
class Subcontext : internal::v1::SubcontextBase
{
    // NOTE: This is likely a better job for variant_storage, since we know based on URI which particular
    // state we're interested in and additionally we'd prefer not to initialize *any* - so in other words
    // somewhere between a union and a variant, which is what variant_storage really is
    using state_type = estd::variant<unknown, Substates...>;

    state_type state_;

    template <ESTD_CPP_CONCEPT(concepts::StateFunctor) F, class ...Args>
    void visit(F&& f, Args&&...args);

public:
    state_type& state() { return state_; }
    constexpr const state_type& state() const { return state_; }

    template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context,
        class F>
    void create(int id_path, Context& context, F&& f);

    template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context>
    void create(int id_path, Context& context)
    {
        create(id_path, context,
            []<class T>(estd::in_place_type_t<T>){ return estd::nullptr_t{}; });
    }

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
