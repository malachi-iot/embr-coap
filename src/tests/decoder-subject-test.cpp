#include <catch.hpp>
#include "../coap-dispatcher.h"
#include "../coap/decoder-subject.hpp"

using namespace moducom::coap;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
IncomingContext test_ctx;
DecoderSubjectBase<experimental::ContextDispatcherHandler> test(test_ctx);
// ---


TEST_CASE("CoAP decoder subject tests", "[coap-decoder-subject]")
{
    SECTION("Test 1")
    {

    }
}
