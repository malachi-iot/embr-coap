#include <driver/ledc.h>

#include <coap/decoder/events.h>
#include <coap/context/from_query.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"
#include "ledc.h"

using namespace embr::coap;

static constexpr ledc_timer_config_t timer_config_default = {
    .speed_mode = LEDC_LS_MODE,           // timer mode
    .duty_resolution = LEDC_DUTY_RESOLUTION, // resolution of PWM duty
    .timer_num = LEDC_LS_TIMER,            // timer index
    .freq_hz = LEDC_FREQ_HZ,                      // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
};


void initialize_ledc()
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = timer_config_default;
        
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
}


Header::Code AppContext::states::ledc_timer::response() const
{
    esp_err_t ret = ledc_timer_config(&config);

    ESP_LOGI(TAG, "completed: got here = ret=%d", ret);

    // TODO: Do BadRequest if we get INVALID_ARG

    return ret == ESP_OK ? Header::Code::Valid : Header::Code::InternalServerError;
}


