#include "unit-test.h"

#include <exp/uripathmap.h>
//#include  <coap/decoder/uri.h>

using namespace moducom;
using namespace uri;

typedef coap::experimental::UriPathMap map_type;

void test_uri_match()
{
    const map_type* result =
            coap::experimental::match(uri::map, 8, "test", id_path_v1_api);

    TEST_ASSERT_EQUAL_INT(id_path_v1_api_test, result->second);
}

void test_uri_match2()
{
    const map_type* result;
    coap::experimental::UriPathMatcher3 matcher(uri::map);

    result = matcher.find("v1");
    TEST_ASSERT_EQUAL_INT(id_path_v1, result->second);

    result = matcher.find("api");
    TEST_ASSERT_EQUAL_INT(id_path_v1_api, result->second);

    result = matcher.find("power");
    TEST_ASSERT_EQUAL_INT(id_path_v1_api_power, result->second);

    TEST_ASSERT_EQUAL_INT(id_path_v1_api_power, *matcher.last_found());
}

#ifdef ESP_IDF_TESTING
TEST_CASE("uri tests", "[uri]")
#else
void test_uri()
#endif
{
    RUN_TEST(test_uri_match);
    RUN_TEST(test_uri_match2);
}