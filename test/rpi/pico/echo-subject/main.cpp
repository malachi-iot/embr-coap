#include <stdio.h>
#include <pico/stdlib.h>

#include <test/support.h>

void udp_coap_init();

int main()
{
    test::v1::init(WIFI_SSID, WIFI_PASSWORD);

    udp_coap_init();

    for(;;) test::v1::cyw43_poll();

    return 0;
}