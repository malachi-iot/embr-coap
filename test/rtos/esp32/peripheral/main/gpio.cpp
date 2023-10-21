#include <embr/platform/esp-idf/gpio.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"

using namespace embr::coap;


void AppContext::states::gpio::on_payload(istream_type& in)
{
    // Since layer1::optional bool is too specialized for
    // >> to treat it well
    int v;

    if(in >> v) level = v;
}

Header::Code AppContext::states::gpio::completed(encoder_type& encoder)
{
    if(!context.uri_int.has_value()) return Header::Code::BadRequest;

    int pin = *context.uri_int;
    Header::Code code = context.header().code();
    embr::esp_idf::gpio gpio((gpio_num_t) pin);
    esp_err_t ret;

    switch(code)
    {
        case Header::Code::Get:
        {
            int val = gpio.level();

            ESP_LOGI(TAG, "read pin=%d: level=%d", pin, val);

            build_reply(context, encoder, Header::Code::Content);

            encoder.option(
                Option::Numbers::ContentFormat,
                Option::ContentFormats::TextPlain);

            encoder.payload();

            auto out = encoder.ostream();

            out << val;

            context.reply(encoder);

            // DEBT: Indicates to not auto deduce return code and instead
            // use code indicated by build_reply.  A little big sloppy and magical
            return Header::Code::Empty;
        }

        case Header::Code::Put:
            // We could do most of this in 'payload' area, but that
            // makes it hard to handle response codes
            gpio.set_direction(GPIO_MODE_OUTPUT);
            ret = gpio.level(*level);
            ESP_LOGI(TAG, "write pin=%d: level=%d", pin, *level);
            return ret == ESP_OK ?
                Header::Code::Changed :
                Header::Code::InternalServerError;

        default:    return Header::Code::NotImplemented;
    }
}
