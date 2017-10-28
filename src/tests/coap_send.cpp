
#include <catch.hpp>

#include "../coap.h"
#include "../coap_transmission.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

TEST_CASE("CoAP message construction tests", "[coap-send]")
{
    SECTION("")
    {
        TestOutgoingMessageHandler out_handler;

        out_handler.send_response(
                CoAP::Header::ResponseCode::Changed,
                (const uint8_t*) "Hello", 5);
    }
}
