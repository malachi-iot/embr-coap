#pragma once

namespace embr { namespace coap { namespace esp_idf {

#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
extern RTC_NOINIT_ATTR uint16_t other_reboot_counter;
extern RTC_NOINIT_ATTR uint16_t panic_reboot_counter;
extern RTC_NOINIT_ATTR uint16_t wdt_reboot_counter;
extern RTC_NOINIT_ATTR uint16_t brownout_reboot_counter;
extern RTC_NOINIT_ATTR uint16_t user_reboot_counter;

esp_reset_reason_t track_reset_reason();

#endif

}}}