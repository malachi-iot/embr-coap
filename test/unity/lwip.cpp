#include "unit-test.h"

#include <embr/platform/lwip/udp.h>

ip_addr_t loopback_addr;

#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
// DEBT: Move this elsewhere
// NOTE: This seems to run once per RUN_TEST
void setUp()
{
    const char* TAG = "setUp";

    ESP_LOGV(TAG, "entry");

    ip_addr_set_loopback(false, &loopback_addr);    // IPv4 loopback
}
#endif
