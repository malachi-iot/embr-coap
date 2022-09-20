#include "esp_log.h"

#include <esp-helper.h>

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

void app_init(void** pcb_recv_arg)
{
    const char* TAG = "app_init";

    ESP_LOGD(TAG, "entry");
}