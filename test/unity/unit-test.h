#include <math.h>

#include <unity.h>

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#define ESP_IDF_TESTING
#endif

#include "../unit-tests/test-data.h"
#include "../unit-tests/test-uri-data.h"

void test_decoder();
void test_encoder();
void test_header();
void test_retry();
void test_uri();