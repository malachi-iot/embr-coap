#include <math.h>

#include <unity.h>

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#define ESP_IDF_TESTING
#endif

#ifdef ESP_IDF_TESTING
// DEBT: Really we want to test against any LwIP capable target - not just from esp-idf
#define LWIP_PRESENT 1
#include "esp_log.h"
#endif


#include "../unit-tests/test-data.h"
#include "../unit-tests/test-uri-data.h"

#if LWIP_PRESENT
#include "lwip/tcpip.h"

#if LWIP_HAVE_LOOPIF && LWIP_LOOPBACK_MAX_PBUFS
#ifndef FEATURE_COAP_LWIP_LOOPBACK_TESTS
#define FEATURE_COAP_LWIP_LOOPBACK_TESTS 1
#endif
#endif

#if FEATURE_COAP_LWIP_LOOPBACK_TESTS
extern ip_addr_t loopback_addr;
#endif

#endif

void test_decoder();
void test_encoder();
void test_experimental();
void test_header();
void test_retry();
void test_rfc7641();
void test_uri();