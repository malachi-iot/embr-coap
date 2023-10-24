#pragma once

#include <estd/istream.h>

#include "../../../context/sub.h"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {

template <class Context>
struct gpio : coap::internal::v1::CoapSubcontextBase::base<Context>
{
    using base_type = coap::internal::v1::CoapSubcontextBase::base<Context>;
    using istreambuf_type = typename Context::istreambuf_type;
    using encoder_type = typename Context::encoder_type;
    using istream_type = estd::detail::basic_istream<istreambuf_type&>;

    static constexpr const char* TAG = "states::gpio";

    static constexpr int id_path = id_path_v1_api_gpio_value;

    constexpr gpio(AppContext& c) : base_type(c) {}

    estd::layer1::optional<bool> level;

    void on_payload(istream_type&);
    Header::Code response() const;
    bool completed(encoder_type&);
};


}}

}}}