#include <stdio.h>
#include <pico/stdlib.h>

#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include "test/support.h"

using namespace estd;

namespace estd {

pico_ostream clog(stdio_usb);

}


namespace test { namespace v1 {

int init(const char* ssid, const char* wifi_password)
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        clog << "failed to initialise" << estd::endl;
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    clog << "connecting to " << ssid << endl;

    //pico_error_codes code;
    int code;

    // DEBT: Something odd about the return code, docs say returns int and also a pico_error_code -
    // AND we get back a nonzero pretty quickly, yet we then get an IP address denoting connectivity,
    // although now that I added 'code', have not seen that nonzero behavior since
    if (code = cyw43_arch_wifi_connect_timeout_ms(ssid, wifi_password, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        clog << "failed to connect: ssid=" << ssid;
        //clog << ", pass=" << WIFI_PASSWORD;
        clog << " with error " << code;
        clog << endl;
        return 1;
    }

    return 0;
}

}}
