#include <driver/ledc.h>

#include <coap/decoder/events.h>

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include "context.h"

using namespace embr::coap;

// Lifted from ledc_fade example in esp-idf

#define LEDC_LS_TIMER           LEDC_TIMER_1
#define LEDC_LS_MODE            LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          5


void initialize_ledc()
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .timer_num = LEDC_LS_TIMER,            // timer index
        .freq_hz = 5000,                      // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Initialize fade service.
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
}


void initialize_ledc_channel(ledc_channel_t channel)
{
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = LEDC_OUTPUT_IO,
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

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


void AppContext::completed_pwm(encoder_type& encoder)
{
    if(header().code() == Header::Code::Put)
    {

    }
}