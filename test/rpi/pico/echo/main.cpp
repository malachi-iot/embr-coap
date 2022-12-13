#include <stdio.h>
#include <pico/stdlib.h>

#include <estd/string.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

void udp_coap_init();


void main_task(__unused void *params)
{
    while (true) {
        static int counter = 0;

        estd::layer1::string<32> s = "Hello, unit test ";

        s += estd::to_string(++counter);

        puts(s.data());

        sleep_ms(5000);
    }
}

int main()
{
    stdio_init_all();

    udp_coap_init();

    for(;;)
    {
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);
    }

    return 0;
}