#pragma once

#include <driver/ledc.h>

#include "ledc.h"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
ledc_timer<Context, id_path_>::ledc_timer(Context& context, const ledc_timer_config_t& def) :
    base_type(context),
    config(def)
{
}


template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
ledc_channel<Context, id_path_>::ledc_channel(
    Context& context,
    const ledc_channel_config_t& def,
    const event::option& e) :
    base_type(context),
    config(def)
{
    populate_uri_value(e);
}


template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
void ledc_channel<Context, id_path_>::on_option(const query& q)
{
    int v;

    if(internal::from_query(q, "pin", v).ec == 0)
    {
        config.gpio_num = v;
        has_config = true;
    }
    else if(internal::from_query(q, "timer_num", v).ec == 0)
    {
        // DEBT: has_config mode we have to validate that 'pin' was specified
        config.timer_sel = (ledc_timer_t)v;
        has_config = true;
    }
}


template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
void ledc_channel<Context, id_path_>::on_payload(istream_type& in)
{
    in >> *duty;
}

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
auto ledc_channel<Context, id_path_>::response() -> code_type
{
    using C = Header::Code;

    if(!uri_value.has_value())    return C::BadRequest;

    // NOTE: Yes, remember the uri value is channel#, gpio is specified
    // as a query option
    config.channel = (ledc_channel_t) *uri_value;

    ESP_LOGI(TAG, "completed: got here: channel=%d duty=%d",
        config.channel,
        *duty);

    if(has_config)
    {
        if(duty.has_value())
            config.duty = *duty;

        esp_err_t ret = ledc_channel_config(&config);

        if(ret != ESP_OK) return C::InternalServerError;
    }
    else if(base_type::header().code() == C::Put)
    {
        if(!duty.has_value())   return C::BadRequest;

        esp_err_t ret = ledc_set_duty_and_update(config.speed_mode, config.channel, *duty, 0);

        if(ret != ESP_OK) return C::InternalServerError;
    }

    return C::Changed;
}

}}

}}}
