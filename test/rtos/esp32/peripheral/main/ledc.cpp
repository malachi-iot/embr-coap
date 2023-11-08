#include "ledc.h"


// Lifted from ledc_fade example in esp-idf

#define LEDC_LS_TIMER           LEDC_TIMER_1
#define LEDC_LS_MODE            LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          5
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_13_BIT
#define LEDC_FREQ_HZ            5000



const ledc_channel_config_t ledc_channel_default = {
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


const ledc_timer_config_t timer_config_default = {
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


