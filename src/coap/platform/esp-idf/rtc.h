#pragma once

namespace embr { namespace coap { namespace esp_idf {

#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
extern RTC_NOINIT_ATTR uint32_t reboot_counter;
#endif

}}}