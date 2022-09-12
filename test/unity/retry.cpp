#include "unit-test.h"

#if __cplusplus >= 201103L
#include <exp/retry.h>

// DEBT: Really we want to test against any LwIP capable target
#ifdef ESP_IDF_TESTING
#include <embr/platform/lwip/transport.hpp>
#endif

#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("retry tests", "[retry]")
#else
void test_retry()
#endif
{
#if __cplusplus >= 201103L
#endif
}