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
#define LEDC_DUTY_RESOLUTION    LEDC_TIMER_13_BIT
#define LEDC_FREQ_HZ            5000


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


// FIX: Actually selecting pin at the moment on channel 0
void AppContext::select_pwm_channel(const event::option& e)
{
    // DEBT: As is the case all over, 'chunk' is assumed to be complete
    // data here
    auto option = (const char*)e.chunk.data();
    
    if(estd::from_chars(option, option + e.chunk.size(), *uri_int).ec == 0)
        ESP_LOGD(TAG, "Selecting pwm gpio # %d", *uri_int);
}


void AppContext::put_pwm(istreambuf_type& streambuf)
{
    if(!uri_int.has_value()) return;

    if(header().code() == Header::Code::Put)
    {
        estd::detail::basic_istream<istreambuf_type&> in(streambuf);

        int val;

        if(in >> val)
        {
            pwm_value = val;

            initialize_ledc_channel(LEDC_CHANNEL_0, *uri_int);
        }
    }
}



void AppContext::completed_pwm(encoder_type& encoder)
{
    if(header().code() == Header::Code::Put)
    {
        bool success = header().code() == Header::Code::Put && uri_int.has_value();

        success &= pwm_value.has_value();

        if(success)
        {
            uint32_t duty = *pwm_value;

            ESP_LOGD(TAG, "completed_pwm: pin %d, duty=%" PRIu32, *uri_int, duty);

            ledc_set_duty_and_update(LEDC_LS_MODE, LEDC_CHANNEL_0, duty, 0);
        }

        response_code = success ? Header::Code::Valid : Header::Code::BadRequest;
    }
}