#include <stdio.h>
#include <pico/stdlib.h>

#include <estd/string.h>
#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include <test/support.h>

void udp_coap_init();

using namespace estd;


// +++ WORKAROUND
// NOTE: I think the fwd of make_encoder with template parameter has caused some inconsistencies

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>

namespace embr::coap {

template <>
LwipIncomingContext::encoder_type make_encoder(LwipIncomingContext&)
{
    return embr::coap::LwipIncomingContext::encoder_type(8);
}

}

// ---

int main()
{
    test::v1::init(WIFI_SSID, WIFI_PASSWORD);

    udp_coap_init();

    for(;;) test::v1::poll();

    return 0;
}