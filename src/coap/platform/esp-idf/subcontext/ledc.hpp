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
void ledc_timer<Context, id_path_>::on_option(const query& q)
{
    int v;

    if(internal::from_query(q, "freq_hz", v).ec == 0)
    {
        config.freq_hz = v;
    }
    else if(internal::from_query(q, "duty_res", v).ec == 0)
    {
        // DEBT: esp-idf the enum matches up, but I don't think
        // that's promised anywhere
        config.duty_resolution = (ledc_timer_bit_t)v;
    }
    else
        bad_option = true;
}


template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
auto ledc_timer<Context, id_path_>::response() const -> code_type
{
    if(bad_option) return Header::Code::BadOption;

    esp_err_t ret = ledc_timer_config(&config);

    ESP_LOGI(TAG, "completed: got here = ret=%d", ret);

    // TODO: Do BadRequest if we get INVALID_ARG

    return ret == ESP_OK ? Header::Code::Valid : Header::Code::InternalServerError;
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
    config.gpio_num = -1;
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
    else
    {
        bad_option = true;
    }
}


template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
void ledc_channel<Context, id_path_>::on_payload(istream_type& in)
{
    // DEBT: Feature flag this as an int for those very tight memory
    // scenarios
    float _duty;

    in >> _duty;
    //in >> *duty;

    if(in.fail())
    {
        duty.reset();
    }
    else
    {
        char c = 0;

        //in >> c;      // FIX: looks like estd isn't able to read in a char this way
        //in.get(c);    // FIX: looks like estd isn't able to read in a char this way
        c = in.get();

        if(in.good())
        {
            ESP_LOGD(TAG, "on_payload: got c=%c",c );
            // TODO: In order to do percentage we need to know resolution, which
            // isn't retained anywhere just yet
        }
        else
        {
            *duty = _duty;
        }
    }
}

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
auto ledc_channel<Context, id_path_>::response() -> code_type
{
    using C = Header::Code;

    if(!uri_value.has_value())  return C::BadRequest;
    if(bad_option)              return C::BadOption;

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
        else
            ESP_LOGD(TAG, "warning: using default duty 0 since none was specified");

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
