#pragma once

// RTOS timer needs a bigger than-default-stack for things to run right
#ifndef FEATURE_WIFI_POLL_USE_RTOS_TIMER
#define FEATURE_WIFI_POLL_USE_RTOS_TIMER 0
#endif

#ifndef WIFI_POLL_TIMEOUT_S
#define WIFI_POLL_TIMEOUT_S (60 * 10)
#endif

#if !FEATURE_WIFI_POLL_USE_RTOS_TIMER
void wifi_retry_poll();
#endif

