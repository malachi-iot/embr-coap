#include "unit-test.h"

#include <exp/uripathmap.h>
//#include  <coap/decoder/uri.h>

using namespace moducom;
using namespace uri;

typedef moducom::coap::experimental::UriPathMap map_type;

void test_uri_match()
{
    const map_type* result =
            coap::experimental::match(uri::map, 8, "test", id_path_v1_api);

    TEST_ASSERT_EQUAL(id_path_v1_api_test, result->second);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("uri tests", "[uri]")
#else
void test_uri()
#endif
{
    RUN_TEST(test_uri_match);
}