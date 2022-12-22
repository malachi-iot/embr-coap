#pragma once

#include <estd/ostream.h>

namespace estd {

extern pico_ostream clog;

}

namespace test {
    
namespace v1 {

int init(const char* ssid, const char* wifi_password);
void poll();

}

}
