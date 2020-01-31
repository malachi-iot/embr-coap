// FIX: Reaches out to mc/mem/platform.h - however that's strongly not desired
// Additionally, it's broken for Blackfin and the ASSERT code I'm not sure where
// to put it, so in the short term patch it up for modern use.  Code disabled 
// until that's done
#include <coap/header.h>

#include "unit-test.h"

using namespace moducom;

void test_default_header()
{
    coap::Header header(coap::Header::Acknowledgement);

    TEST_ASSERT_EQUAL(coap::Header::Acknowledgement, header.type());
    TEST_ASSERT_EQUAL(1, header.version());
}

#ifdef ESP_IDF_TESTING
TEST_CASE("header tests", "[header]")
#else
void test_header()
#endif
{
    RUN_TEST(test_default_header);
}