#pragma once

#include <estd/istream.h>

#include "../../../context/sub.h"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {


// DEBT: I'm thinking a wrapper to handle the id_path portion would be better
// DEBT: We want ReplyContext here not IncomingContext - somehow that fails though
template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
struct gpio : coap::internal::v1::SubcontextBase::base<Context>,
    // DEBT: Not used yet, but getting there - decouple from tracking
    // uri_int in Context
    coap::experimental::UriValuePipeline<int, -1>
{
    using base_type = coap::internal::v1::SubcontextBase::base<Context>;
    using istreambuf_type = typename Context::istreambuf_type;
    using encoder_type = typename Context::encoder_type;
    using istream_type = estd::detail::basic_istream<istreambuf_type&>;

    static constexpr const char* TAG = "subcontext::gpio";

    static constexpr int id_path = id_path_;

    constexpr gpio(Context& c) : base_type(c) {}
    gpio(Context& c, const event::option&);

    estd::layer1::optional<bool> level;

    void on_payload(istream_type&);
    Header::Code response() const;
    bool completed(encoder_type&);
};


}}

}}}