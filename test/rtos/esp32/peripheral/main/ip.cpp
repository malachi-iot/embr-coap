#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include <estd/port/freertos/timer.h>

// This file is in service of https://github.com/malachi-iot/mc-coap/issues/9
// NOTE: It's presumed that re-request of DHCP stops at a certain point, or
// doesn't auto-start after it's lost

static void dhcp_retry_callback(TimerHandle_t pxTimer)
{
    static const char* TAG = "dhcp_retry_callback";

    ESP_LOGD(TAG, "reached here");

    // TODO: Now re-request a DHCP address here
}

static estd::freertos::timer<true> dhcp_rety_timer(
    "DHCP retry",
    estd::chrono::seconds(60),
    //estd::chrono::seconds(600 * 5),
    true,
    nullptr,
    dhcp_retry_callback);

static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    static constexpr estd::chrono::milliseconds timeout(50);

    if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                dhcp_rety_timer.stop(timeout);
                break;

            case IP_EVENT_STA_LOST_IP:
                dhcp_rety_timer.reset(timeout);
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
