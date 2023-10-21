#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>

#include "context.h"
#include "features.h"

#include <coap/decoder/events.h>

// DEBT: See debt in gpio.cpp on this one
#include <coap/platform/lwip/encoder.h>


// Guidance from https://github.com/espressif/esp-idf/tree/v5.1.1/examples/peripherals/adc/oneshot_read

static constexpr const char* TAG = "app::adc";

using namespace embr::coap;

#if FEATURE_APP_ANALOG_IN
static adc_oneshot_unit_handle_t adc1_handle;

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define ADC_CHANNEL ADC_CHANNEL_0
#elif defined(CONFIG_IDF_TARGET_ESP32)
#define ADC_CHANNEL ADC_CHANNEL_0
#else
#define ADC_CHANNEL ADC_CHANNEL_8
#endif

void initialize_adc()
{
    ESP_LOGD(TAG, "initialize_adc: entry");

    constexpr adc_oneshot_unit_init_cfg_t init_config1 =
    {
        .unit_id = ADC_UNIT_1,
    #if SOC_ADC_RTC_CTRL_SUPPORTED
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    #else
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    #endif
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config =
    {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    // DEBT: Make this channel configurable, and possibly many of them
    // For now, we specifically are interested in IO9 analog in since RejsaCAN 12V divider is there
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    // DEBT: Need calibration.  For example a curve fit scheme is availbale which seems fitting
    // being that ESP32 ADCs like to squish at the edges
}

Header::Code AppContext::states::analog::response() const
{
    if(context.header().code() == Header::Code::Get)
        return Header::Code::Empty;
    else
        return Header::Code::BadRequest;
}

bool AppContext::states::analog::completed(encoder_type& encoder)
{
    {
        int raw;
        esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);

        if(ret != ESP_OK)
        {
            build_reply(context, encoder, Header::Code::InternalServerError);
            return true;
        }

        build_reply(context, encoder, Header::Code::Content);

        encoder.option(
            Option::Numbers::ContentFormat,
            Option::ContentFormats::TextPlain);

        encoder.payload();

        auto out = encoder.ostream();

        out << raw;

        context.reply(encoder);
    }

    return true;
}

#else
void initialize_adc()
{
    ESP_LOGD(TAG, "No ADC mode enabled");
}
#endif
