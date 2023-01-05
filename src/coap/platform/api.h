#pragma once

#ifdef ESP_PLATFORM
#include "esp-idf/api.h"
#endif

namespace embr { namespace coap {

namespace sys_paths { namespace v1 {

enum _paths
{
    root = 1000,    // general runtime stats
    root_firmware,  // firmware info
    root_uptime,    // uptime specifically
    root_reboot     // reboot command
};


}}

}}