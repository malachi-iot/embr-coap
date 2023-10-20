#include <embr/platform/esp-idf/gpio.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"

using namespace embr::coap;

//static const char* TAG = "AppContext::gpio";

// NOTE: Expects to run at 'option' event.  Not 'option_completed' because we need
// to extract the particular GPIO value when it appears
void AppContext::select_gpio(const event::option& e)
{
    // DEBT: As is the case all over, 'chunk' is assumed to be complete
    // data here
    auto option = (const char*) e.chunk.data();
    
    if(estd::from_chars(option, option + e.chunk.size(), *gpio.pin).ec == 0)
        ESP_LOGD(TAG, "Selecting gpio # %d", *gpio.pin);
}


// NOTE: Expects to run at 'streambuf_payload' event
void AppContext::put_gpio(istreambuf_type& streambuf)
{
    if(!gpio.pin.has_value()) return;

    if(header().code() == Header::Code::Put)
    {
        estd::detail::basic_istream<istreambuf_type&> in(streambuf);

        int val;

        if(in >> val)
        {
            auto pin = (gpio_num_t)*gpio.pin;

            ESP_LOGI(TAG, "gpio: set #%d to %d", pin, val);

            embr::esp_idf::gpio gpio(pin);

            gpio.set_direction(GPIO_MODE_OUTPUT);
            gpio.level(val);
        }
        else
        {
            gpio.pin.reset();
            ESP_LOGW(TAG, "gpio: could not set value, invalid payload");
        }
    }
    else
        ESP_LOGW(TAG, "gpio: undefined behavior - payload present, but not a put");
}


// Runs at 'completed' event and generates a response
// If called during 'put', only returns code matching success of previous put operation
// If called during 'get', commences with get operation
// In other cases, returns a 'bad request'
void AppContext::completed_gpio(encoder_type& encoder)
{
    // TODO: Detect if no payload was present during a put operation and indicate bad request
    // Slightly tricky at the moment as we don't get a payload event at all when there is none (naturally)
    // meaning we'd have to keep an extra flag around to tag that we got one.  However, that makes
    // more sense at the Decoder state machine level, where it may be zero-cost to do that

    if(header().code() == Header::Code::Get)
    {
        if(!gpio.pin.has_value())
        {
            response_code = Header::Code::BadRequest;
            return;
        }

        auto pin = (gpio_num_t)gpio.pin.value();

        ESP_LOGI(TAG, "gpio: get %d", pin);

        embr::esp_idf::gpio gpio(pin);

        int val = gpio.level();

        build_reply(*this, encoder, Header::Code::Content);

        encoder.option(
            Option::Numbers::ContentFormat,
            Option::ContentFormats::TextPlain);

        encoder.payload();

        auto out = encoder.ostream();

        out << val;

        reply(encoder);
    }
    else
    {
        bool success = header().code() == Header::Code::Put && gpio.pin.has_value();
        response_code = success ? Header::Code::Valid : Header::Code::BadRequest;
    }
}


void AppContext::states::gpio::on_payload(istream_type& in)
{
    // Since layer1::optional bool is too specialized for
    // >> to treat it well
    int v;

    if(in >> v) level = v;
}

Header::Code AppContext::states::gpio::completed(encoder_type& encoder)
{
    if(!context.uri_int.value()) return Header::Code::BadRequest;

    int pin = *context.uri_int;
    Header::Code code = context.header().code();
    embr::esp_idf::gpio gpio((gpio_num_t) pin);
    esp_err_t ret;

    switch(code)
    {
        case Header::Code::Get:
        {
            int val = gpio.level();

            ESP_LOGI(TAG, "gpio: pin=%d level=%d", pin, val);

            build_reply(context, encoder, Header::Code::Content);

            encoder.option(
                Option::Numbers::ContentFormat,
                Option::ContentFormats::TextPlain);

            encoder.payload();

            auto out = encoder.ostream();

            out << val;

            context.reply(encoder);
            return Header::Code::Empty;
        }

        case Header::Code::Put:
            // We could do most of this in 'payload' area, but that
            // makes it hard to handle response codes
            gpio.set_direction(GPIO_MODE_OUTPUT);
            ret = gpio.level(*level);
            return ret == ESP_OK ?
                Header::Code::Changed :
                Header::Code::InternalServerError;

        default:    return Header::Code::NotImplemented;
    }
}
