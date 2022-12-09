#include <embr/platform/esp-idf/gpio.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"

using namespace embr::coap;

static const char* TAG = "AppContext::gpio";

// NOTE: Expects to run at 'option' event
void AppContext::select_gpio(const event::option& e)
{
    int& pin = gpio.pin;

    // DEBT: As is the case all over, 'chunk' is assumed to be complete
    // data here
    auto option = (const char*) e.chunk.data();
    
    if(estd::from_chars<int>(option, option + e.chunk.size(), pin).ec == 0)
        ESP_LOGD(TAG, "Selecting gpio # %d", pin);
    else
        pin = -1;
}

// NOTE: Expects to run at 'streambuf_payload' event
void AppContext::put_gpio(istreambuf_type& streambuf)
{
    if(gpio.pin == -1) return;

    if(header().code() == Header::Code::Put)
    {
        //auto& s = e.streambuf;
        // DEBT: I think this can be promoted out of internal, I believe I put
        // this in there way back when because the signature doesn't match std - but
        // by this point, that's a feature not a bug
        estd::internal::basic_istream<istreambuf_type&> in(streambuf);

        int val = -1;

        in >> val;

        ESP_LOGI(TAG, "gpio: set #%d to %d", gpio.pin, val);

        embr::esp_idf::gpio gpio((gpio_num_t)this->gpio.pin);

        gpio.set_direction(GPIO_MODE_OUTPUT);
        gpio.level(val);
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
    if(header().code() == Header::Code::Get)
    {
        ESP_LOGI(TAG, "gpio: get %d", gpio.pin);

        embr::esp_idf::gpio gpio((gpio_num_t)this->gpio.pin);

        int val = gpio.level();

        build_reply(*this, encoder, Header::Code::Content);

        encoder.option(
            Option::Numbers::ContentFormat,
            Option::ContentFormats::TextPlain);

        encoder.payload();

        auto out = encoder.ostream();

        out << val;
    }
    else if(header().code() == Header::Code::Put && gpio.pin != -1)
        build_reply(*this, encoder, Header::Code::Valid);
    else
        build_reply(*this, encoder, Header::Code::BadRequest);
}
