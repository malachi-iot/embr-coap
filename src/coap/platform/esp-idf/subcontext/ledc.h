#pragma once

#include <driver/ledc.h>

#include <estd/istream.h>

#include "../../../context/sub.h"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {

// DEBT: Carrying forward same DEBT we see in gpio.h

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
struct ledc_timer : coap::internal::v1::SubcontextBase::base<Context>
{
    using base_type = coap::internal::v1::SubcontextBase::base<Context>;
    using typename base_type::query;
    using typename base_type::code_type;
    using istreambuf_type = typename Context::istreambuf_type;
    using encoder_type = typename Context::encoder_type;
    using istream_type = estd::detail::basic_istream<istreambuf_type&>;

    static constexpr const char* TAG = "subcontext::ledc_timer";

    static constexpr int id_path = id_path_;

    ledc_timer_config_t config;

    ledc_timer(Context&, const ledc_timer_config_t&);

    void on_option(const query&);
    code_type response() const;
};

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
struct ledc_channel : coap::internal::v1::SubcontextBase::base<Context>
{
    using base_type = coap::internal::v1::SubcontextBase::base<Context>;
    using typename base_type::query;
    using typename base_type::code_type;
    using istreambuf_type = typename Context::istreambuf_type;
    using encoder_type = typename Context::encoder_type;
    using istream_type = estd::detail::basic_istream<istreambuf_type&>;

    static constexpr const char* TAG = "subcontext::ledc_channel";

    static constexpr int id_path = id_path_;

    ledc_channel_config_t config;

    // We always use above config, but if this is true it signals a call
    // to channel configure itself vs a mere duty update
    bool has_config = false;

    estd::layer1::optional<uint16_t, 0xFFFF> duty;

    ledc_channel(AppContext&);

    void on_option(const query&);
    void on_payload(istream_type&);
    code_type response();
};

}}

}}}
