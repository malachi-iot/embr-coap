#pragma once

#ifdef ESP_PLATFORM
#include "esp-idf/api.h"
#endif

namespace embr { namespace coap {

namespace sys_paths { namespace v1 {

enum _paths
{
    root = 1000,    // general runtime stats
    root_firmware,  // firmware summary info
    root_uptime,    // uptime specifically
    root_memory,    // detailed memory info
    root_reboot,    // reboot command

    root_firmware_version,  // firmware full version info

    well_known = 2000,
    well_known_core,
};


}}

#define EMBR_COAP_V1_SYS_PATHS(id_parent) \
    { "sys",        embr::coap::sys_paths::v1::root,            id_parent },\
    { "firmware",   embr::coap::sys_paths::v1::root_firmware,   embr::coap::sys_paths::v1::root },\
    { "version",    embr::coap::sys_paths::v1::root_firmware_version,   embr::coap::sys_paths::v1::root_firmware },  \
    { "mem",        embr::coap::sys_paths::v1::root_memory,     embr::coap::sys_paths::v1::root },\
    { "reboot",     embr::coap::sys_paths::v1::root_reboot,     embr::coap::sys_paths::v1::root }

// Not ready yet
#define EMBR_COAP_CoRE_PATHS()  \
    { ".well-known",    sys_paths::well_known,         MCCOAP_URIPATH_NONE },\
    { "core",           sys_paths::well_known_core,    sys_paths::well_known }


}}