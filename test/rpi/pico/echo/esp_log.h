// DEBT: Simplistic wrapper to bring things online for rpi pico
#pragma once

#include <stdio.h>

#define ESP_LOGW(tag, fmt...)     { printf("W %s: ", tag); printf(fmt); puts(""); }
#define ESP_LOGI(tag, fmt...)     { printf("I %s: ", tag); printf(fmt); puts(""); }
#define ESP_LOGD(tag, fmt...)     { printf("D %s: ", tag); printf(fmt); puts(""); }
#define ESP_LOGV(tag, fmt...)     { printf("V %s: ", tag); printf(fmt); puts(""); }

#define ESP_LOG_BUFFER_HEX(tag, buffer, len)