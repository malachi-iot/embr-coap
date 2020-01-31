// TODO: NetBuf encoder still uses mk. 1 netbuf which we don't want to invest
// time into, so revise that so that we can code a test here
#include "unit-test.h"

#ifdef ESP_IDF_TESTING
TEST_CASE("encoder tests", "[encoder]")
#else
void test_encoder()
#endif
{

}