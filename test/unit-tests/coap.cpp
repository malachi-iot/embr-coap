#include <catch2/catch.hpp>

#include "coap.h"
#include "coap_transmission.h"
#include "coap-encoder.h"
#include "test-data.h"

#define CLEANUP_COAP_CPP
#define CLEANUP

using namespace embr::coap;

static void empty_fn()
{

}




TEST_CASE("CoAP tests", "[coap]")
{
    SECTION("Basic generating single option")
    {
        uint8_t buffer[64];
        int counter = 0;

        experimental::layer2::OptionBase o(1);

        o.length = 1;
        o.value_string = "a";

        OptionEncoder sm(o);

        while(sm.state() != Option::OptionValueDone)
        {
            buffer[counter++] = sm.generate();
        }

        REQUIRE(buffer[0] == 0x11);
        REQUIRE(buffer[1] == 'a');
        REQUIRE(counter == 2);
    }
    SECTION("Basic generating single option: 16-bit")
    {
        uint8_t buffer[64];
        int counter = 0;

        experimental::layer2::OptionBase o(10000);

        o.length = 1;
        o.value_string = "a";

#ifdef CLEANUP
        OptionEncoder sm(o);
#else
        experimental::layer2::OptionGenerator::StateMachine sm(o);
#endif

        while(sm.state() != Option::OptionValueDone)
        {
            buffer[counter++] = sm.generate();
        }

        REQUIRE(buffer[0] == 0xE1);
        REQUIRE(buffer[1] == ((10000 - 269) >> 8));
        REQUIRE(buffer[2] == ((10000 - 269) & 0xFF));
        REQUIRE(buffer[3] == 'a');
        REQUIRE(counter == 4);
    }
}
