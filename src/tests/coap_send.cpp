
#include <catch.hpp>

#include "../coap.h"
#include "../coap_transmission.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

TEST_CASE("CoAP message construction tests", "[coap-send]")
{
    SECTION("")
    {
        uint8_t expected[] =
        {
            0x60, // header v1 with ACK (piggyback response does this) and no TKL
            0x44, // response code of Changed = binary 010 00100 = 2.04
            0x00, 0x00, // message ID of 0
            0xFF, // payload marker
            'H', 'e', 'l', 'l', 'o'
        };

        OutgoingResponseHandler out_handler;

        out_handler.send(
                CoAP::Header::ResponseCode::Changed,
                (const uint8_t*) "Hello", 5);

        const uint8_t* result = out_handler.get_buffer();

        for(int i = 0; i < sizeof(expected); i++)
        {
            INFO("At position " << i);

            uint8_t result_ch = result[i];
            uint8_t expected_ch = expected[i];

            REQUIRE(expected_ch == result_ch);
        }
    }
}
