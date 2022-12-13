#include <stdio.h>
#include <pico/stdlib.h>

#include <estd/string.h>
#include <estd/ostream.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

void udp_coap_init();

using namespace estd;

static pico_ostream clog(stdio_usb);

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

    if (cyw43_arch_init())
    {
        clog << "failed to initialise" << estd::endl;
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    clog << "connecting to " << WIFI_SSID << endl;

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        clog << "failed to connect: ssid=" << WIFI_SSID;
        //clog << ", pass=" << WIFI_PASSWORD;
        clog << endl;
        return 1;
    }

    udp_coap_init();

    const ip4_addr_t* addr = nullptr;

    for(;;)
    {
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for WiFi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        sleep_ms(1);

        if(addr == nullptr && netif_is_link_up(netif_default))
        {
            if(!ip4_addr_isany_val(*netif_ip4_addr(netif_default)))
            {
                addr = netif_ip4_addr(netif_default);

                char temp[32];
                ip4addr_ntoa_r(addr, temp, sizeof(temp));

                clog << "Got IP: " << temp << endl;
            }
        }
    }

    return 0;
}