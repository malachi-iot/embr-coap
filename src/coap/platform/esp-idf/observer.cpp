#include "observer.h"

namespace embr { namespace coap {

const char* VersionObserverBase::TAG = "VersionObserver";

namespace esp_idf {

// DEBT: It's up to external party to increment this
// DEBT: Doesn't belong in observer.cpp or even necessarily CoAP (maybe root embr would be better?)
#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
RTC_NOINIT_ATTR uint16_t other_reboot_counter;
RTC_NOINIT_ATTR uint16_t panic_reboot_counter;
RTC_NOINIT_ATTR uint16_t wdt_reboot_counter;
RTC_NOINIT_ATTR uint16_t brownout_reboot_counter;
RTC_NOINIT_ATTR uint16_t user_reboot_counter;

esp_reset_reason_t track_reset_reason()
{
    static const char* TAG = "track_reset_reason";
    const esp_reset_reason_t reset_reason = esp_reset_reason();

    // As per
    // https://stackoverflow.com/questions/69880289/c-int-undefined-behaviour-when-stored-as-rtc-noinit-attr-esp32
    switch(reset_reason)
    {
        case ESP_RST_UNKNOWN:       // ESP32C3 starts after flash with this reason
        case ESP_RST_EXT:
        case ESP_RST_POWERON:
            brownout_reboot_counter = 0;
            panic_reboot_counter = 0;
            wdt_reboot_counter = 0;
            user_reboot_counter = 0;
            other_reboot_counter = 0;
            ESP_LOGD(TAG, "Poweron");
            break;

        case ESP_RST_PANIC:
            ++panic_reboot_counter;
            break;

        case ESP_RST_BROWNOUT:
            ++brownout_reboot_counter;
            break;

        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
            ++wdt_reboot_counter;
            break;

        case ESP_RST_DEEPSLEEP:
        case ESP_RST_SW:
            ++user_reboot_counter;
            break;

        default:
            ++other_reboot_counter;
            break;
    }

    return reset_reason;
}

#endif

}

}}