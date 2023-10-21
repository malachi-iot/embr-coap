#include <driver/ledc.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"
#include "from_query.h"

using namespace embr::coap;

// Lifted from ledc_fade example in esp-idf

#define LEDC_LS_TIMER           LEDC_TIMER_1
#define LEDC_LS_MODE            LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          5
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_13_BIT
#define LEDC_FREQ_HZ            5000

static constexpr ledc_timer_config_t timer_config_default = {
    .speed_mode = LEDC_LS_MODE,           // timer mode
    .duty_resolution = LEDC_DUTY_RESOLUTION, // resolution of PWM duty
    .timer_num = LEDC_LS_TIMER,            // timer index
    .freq_hz = LEDC_FREQ_HZ,                      // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
};


static constexpr ledc_channel_config_t ledc_channel_default = {
    .gpio_num       = 0,
    .speed_mode     = LEDC_LS_MODE,
    .channel        = LEDC_CHANNEL_0,
    .intr_type      = LEDC_INTR_DISABLE,
    .timer_sel      = LEDC_LS_TIMER,
    .duty           = 0, // Set duty to 0%
    .hpoint         = 0,
    .flags {
        .output_invert = 0
    }
};


void initialize_ledc()
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .duty_resolution = LEDC_DUTY_RESOLUTION, // resolution of PWM duty
        .timer_num = LEDC_LS_TIMER,            // timer index
        .freq_hz = LEDC_FREQ_HZ,                      // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Initialize fade service.
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
}

/*
void initialize_ledc_channel(ledc_channel_t channel, int gpio)
{
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpio,
        .speed_mode     = LEDC_LS_MODE,
        .channel        = channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_LS_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .flags {
            .output_invert = 0
        }
    };

    // NOTE: Be advised, gpio gathers up on a channel and I haven't found a way to clear it out.
    // So expect more and more gpios to participate in channel 0 PWM as runtime continues.  
    // This ledc_stop doesn't really help that it seems.
    ledc_stop(LEDC_LS_MODE, channel, 0);

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
*/

AppContext::states::ledc_timer::ledc_timer(AppContext& context) : base{context}
{
    config = timer_config_default;
}

void AppContext::states::ledc_timer::on_option(const query& q)
{
    int v;

    if(from_query(q, "freq_hz", v).ec == 0)
    {
        config.freq_hz = v;
    }
    else if(from_query(q, "duty_res", v).ec == 0)
    {
        // DEBT: esp-idf the enum matches up, but I don't think
        // that's promised anywhere
        config.duty_resolution = (ledc_timer_bit_t)v;
    }
}


Header::Code AppContext::states::ledc_timer::completed(encoder_type& encoder)
{
    esp_err_t ret = ledc_timer_config(&config);

    ESP_LOGI(TAG, "completed: got here = ret=%d", ret);

    // TODO: Do BadRequest if we get INVALID_ARG

    return ret == ESP_OK ? Header::Code::Valid : Header::Code::InternalServerError;
}


AppContext::states::ledc_channel::ledc_channel(AppContext& context) : base{context}
{
    // TODO: Consider passing in channel # right away, which would be a greater commitment to
    // channel-on-uri vs channel-on-query

    //config.channel.channel = (ledc_channel_t) channel;
    //config.channel = LEDC_CHANNEL_0;
    //config.timer_sel = LEDC_LS_TIMER;

    config = ledc_channel_default;
    config.gpio_num = -1;
}

void AppContext::states::ledc_channel::on_option(const query& q)
{
    int v;

    if(from_query(q, "pin", v).ec == 0)
    {
        config.gpio_num = v;
        has_config = true;
    }
    else if(from_query(q, "timer_num", v).ec == 0)
    {
        // DEBT: has_config mode we have to validate that 'pin' was specified
        config.timer_sel = (ledc_timer_t)v;
        has_config = true;
    }
}


void AppContext::states::ledc_channel::on_payload(istream_type& in)
{
    in >> *duty;
}


Header::Code AppContext::states::ledc_channel::completed(encoder_type& encoder)
{
    if(!context.uri_int.has_value())
    {
        return Header::Code::BadRequest;
    }

    Header::Code code = context.header().code();
    esp_err_t ret;

    config.channel = (ledc_channel_t) *context.uri_int;
    
    ESP_LOGI(TAG, "completed: got here: channel=%d duty=%d",
        config.channel,
        *duty);

    if(has_config)
    {
        ret = ledc_channel_config(&config);

        if(ret != ESP_OK) return Header::Code::InternalServerError;
    }

    if(code == Header::Code::Put)
    {
        if(!duty.has_value())   return Header::Code::BadRequest;

        ret = ledc_set_duty_and_update(config.speed_mode, config.channel, *duty, 0);

        if(ret != ESP_OK) return Header::Code::InternalServerError;
    }

    return Header::Code::Valid;
}



