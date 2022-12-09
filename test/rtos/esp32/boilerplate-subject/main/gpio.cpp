#include <embr/platform/esp-idf/gpio.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"

using namespace embr::coap;

static const char* TAG = "AppContext::gpio";

// NOTE: Expects to run at 'option' event.  Not 'option_completed' because we need
// to extract the particular GPIO value when it appears
void AppContext::select_gpio(const event::option& e)
{
    // DEBT: As is the case all over, 'chunk' is assumed to be complete
    // data here
    auto option = (const char*) e.chunk.data();
    
    if(estd::from_chars(option, option + e.chunk.size(), gpio.pin).ec == 0)
        ESP_LOGD(TAG, "Selecting gpio # %d", gpio.pin);
    else
        gpio.pin = -1;
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

        int val;

        if(in >> val)
        {
            ESP_LOGI(TAG, "gpio: set #%d to %d", gpio.pin, val);

            embr::esp_idf::gpio gpio((gpio_num_t)this->gpio.pin);

            gpio.set_direction(GPIO_MODE_OUTPUT);
            gpio.level(val);
        }
        else
        {
            gpio.pin = -1;
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
