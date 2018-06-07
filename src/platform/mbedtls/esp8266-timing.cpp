// Yanked in from PGESP-4
#if defined(ESP8266) || defined(IDF_VER)

#include <mbedtls/timing.h>

// If need be, we can access IDF-VER ala https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/make/project.mk
#ifdef OLD_STYLE
#include "esp_common.h"
#else
#include "esp_misc.h"
#include "esp_sta.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

//#define DEBUG

// Worked for esp-open-rtos but not for ESP8266_RTOS_SDK
// (though eventually estdlib might fix that...)
#ifdef DEBUG_STD
#include <iostream>

using namespace std;
#endif

#include <string.h>

static uint32_t get_current_time()
{
    // lifted from https://bbs.espressif.com/viewtopic.php?t=4835
    // and not tested
    return xTaskGetTickCount() * portTICK_RATE_MS;
    // lifted from esp-open-rtos samples
    //return timer_get_count(FRC2) / 5000; // to get roughly 1 ms resolution
    return pdMS_TO_TICKS(xTaskGetTickCount());
}

#define TIMING_METHOD_2

// As initially described by https://tls.mbed.org/kb/how-to/dtls-tutorial
extern "C" void mbedtls_timing_set_delay(void* data, uint32_t int_ms, uint32_t fin_ms)
{
    mbedtls_timing_delay_context* ctx = (mbedtls_timing_delay_context*) data;
    auto current_time = get_current_time();
#ifdef TIMING_METHOD_2
    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    memcpy(&ctx->timer, &current_time, sizeof(current_time));
#else
    ctx->int_ms = current_time + int_ms;
    ctx->fin_ms = fin_ms == 0 ? 0 : current_time + fin_ms;
#endif
#ifdef DEBUG
    printf("mbedtls_timing_set_delay current seconds: %u (%u)\n", (uint16_t)(current_time / 1000), current_time);
#elif defined(DEBUG_STD)
    clog << __func__ << " current seconds: " << (uint16_t)(current_time / 1000) << endl;
#endif
}




extern "C" int mbedtls_timing_get_delay(void* data)
{
    mbedtls_timing_delay_context* ctx = (mbedtls_timing_delay_context*) data;
    if(ctx->fin_ms == 0) return -1;

    auto current_time = get_current_time();

#ifdef DEBUG_STD
    clog << __func__ << " current seconds: " << (uint16_t)(current_time / 1000) << endl;
#elif defined(DEBUG)
    printf("mbedtls_timing_get_delay current seconds: %u (%u)\n", (uint16_t)(current_time / 1000), current_time);
#endif

#ifdef TIMING_METHOD_2
    uint32_t last_time;

    memcpy(&last_time, &ctx->timer, sizeof(last_time));

    current_time -= last_time;
#endif
    if(current_time > ctx->fin_ms) return 2;
    else if(current_time > ctx->int_ms) return 1;
    else return 0;
}

#endif
