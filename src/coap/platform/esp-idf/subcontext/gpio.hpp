#pragma once

#include <embr/platform/esp-idf/gpio.h>

#include "gpio.h"
#include "../../../context/sub.hpp"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {


template <class Context>
void gpio<Context>::on_payload(istream_type& in)
{
    // Since layer1::optional bool is too specialized for
    // >> to treat it well
    int v;

    if(in >> v) level = v;
}


template <class Context>
Header::Code gpio<Context>::response() const
{
    // DEBT: Need to filter by Context that even *has* uri_int -OR- switch
    // arch over to placement-grow type of variant and track uri_int within intermediate
    // base class of this subcontext
    return base_type::context.uri_int.has_value() ?
        Header::Code::Empty :
        Header::Code::BadRequest;
}

template <class Context>
bool gpio<Context>::completed(encoder_type& encoder)
{
    Context& c = base_type::context;

    int pin = *c.uri_int;
    embr::esp_idf::gpio gpio((gpio_num_t) pin);

    switch(c.header().code())
    {
        case Header::Code::Get:
        {
            int val = gpio.level();

            ESP_LOGI(TAG, "read pin=%d: level=%d", pin, val);

            build_reply(c, encoder, Header::Code::Content);

            encoder.option(
                Option::Numbers::ContentFormat,
                Option::ContentFormats::TextPlain);

            encoder.payload();

            auto out = encoder.ostream();

            out << val;

            c.reply(encoder);
            break;
        }

        case Header::Code::Put:
        {
            // We could do most of this in 'payload' area, but that
            // makes it hard to handle response codes
            int code;

            if(level.has_value())
            {
                gpio.set_direction(GPIO_MODE_OUTPUT);
                esp_err_t ret = gpio.level(*level);
                ESP_LOGI(TAG, "write pin=%d: level=%d", pin, *level);
                code = (ret == ESP_OK) ?
                    Header::Code::Changed :
                    Header::Code::InternalServerError;
            }
            else
                code = Header::Code::BadRequest;

            build_reply(c, encoder, code);
            c.reply(encoder);
            break;
        }

        default:    return false;
    }

    return true;
}



}}

}}}