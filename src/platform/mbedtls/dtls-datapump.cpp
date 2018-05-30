//
// Created by malachi on 5/30/18.
//

// Adapting from https://github.com/ARMmbed/mbedtls/blob/development/programs/ssl/dtls_server.c

namespace moducom { namespace coap {
#ifdef FEATURE_MCCOAP_MBEDTLS

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf     printf
#define mbedtls_fprintf    fprintf
#define mbedtls_time_t     time_t
#endif

void dtls_setup();
void dtls_loop();

#endif
}}