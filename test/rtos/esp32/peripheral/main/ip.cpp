#include <esp_event.h>
#include <esp_wifi.h>

// This file is in service of https://github.com/malachi-iot/mc-coap/issues/9

static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                break;

            case IP_EVENT_STA_LOST_IP:
                break;

            default:
                break;
        }
    }
}

void initialize_ip_retry()
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
}
