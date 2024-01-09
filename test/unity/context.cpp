#include <coap/context.h>

static void test_subcontext()
{

}

#ifdef ESP_IDF_TESTING
TEST_CASE("context tests", "[context]")
#else
void test_context()
#endif
{
    RUN_TEST(test_subcontext);
}