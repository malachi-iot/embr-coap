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
    root_memory,    // detailed memory info
    root_reboot,    // reboot command

    well_known = 2000,
    well_known_core,
};


}}

// DEBT: Should fully qualify these namespaces
#define EMBR_COAP_V1_SYS_PATHS(id_parent) \
    { "sys",        sys_paths::v1::root,            id_parent },\
    { "firmware",   sys_paths::v1::root_firmware,   sys_paths::v1::root },\
    { "mem",        sys_paths::v1::root_memory,     sys_paths::v1::root },\
    { "reboot",     sys_paths::v1::root_reboot,     sys_paths::v1::root },\
    { "version",    sys_paths::v1::root_firmware,   sys_paths::v1::root }

// Not ready yet
#define EMBR_COAP_CoRE_PATHS()  \
    { ".well-known",    sys_paths::well_known,         MCCOAP_URIPATH_NONE },\
    { "core",           sys_paths::well_known_core,    sys_paths::well_known }


}}