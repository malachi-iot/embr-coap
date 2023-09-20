#include "observer.h"

namespace embr { namespace coap {

const char* VersionObserverBase::TAG = "VersionObserver";

namespace esp_idf {

// DEBT: It's up to external party to increment this
// DEBT: Doesn't belong in observer.cpp
#ifdef CONFIG_EMBR_COAP_RTC_RESTART_COUNTER
RTC_NOINIT_ATTR uint32_t reboot_counter;
RTC_NOINIT_ATTR uint32_t panic_reboot_counter;
RTC_NOINIT_ATTR uint32_t wdt_reboot_counter;
RTC_NOINIT_ATTR uint32_t brownout_reboot_counter;
#endif

}

}}