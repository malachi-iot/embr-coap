#include <coap/context.h>

#include "unit-test.h"

using namespace embr::coap;

// DEBT: Make a consolidated test code base for catch & unity for things
// like this
namespace test {

char synthetic_outbuf[128];

}

static internal::UriPathMap uri_map[] =
{
    { "v1", test::uris::v1, MCCOAP_URIPATH_NONE }
};


#if FEATURE_EMBR_COAP_SUBCONTEXT
static void test_subcontext()
{
    SyntheticIncomingContext<true> context(uri_map);

    subcontext::Subcontext<
        test::subcontext1,
        test::subcontext2<>> sc;

    auto encoder = make_encoder(context);
    sc.on_completed(encoder, context);

    TEST_ASSERT_EQUAL_INT(0, sc.state().index());
}
#endif

#ifdef ESP_IDF_TESTING
TEST_CASE("context tests", "[context]")
#else
void test_context()
#endif
{
#if FEATURE_EMBR_COAP_SUBCONTEXT
    RUN_TEST(test_subcontext);
#endif
}