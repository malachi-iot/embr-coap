#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include <estd/port/freertos/timer.h>

#include "ip.h"

// This file is in service of https://github.com/malachi-iot/mc-coap/issues/9
// NOTE: It's presumed that re-request of DHCP stops at a certain point, or
// doesn't auto-start after it's lost

extern esp_netif_t* wifi_netif;

static int last_ip_event_id;
static int last_wifi_event_id;

static bool got_ip = false;
//static bool got_wifi = false;

// DEBT: Put into embr
const char* to_string(esp_netif_dhcp_status_t status)
{
    switch(status)
    {
        case ESP_NETIF_DHCP_INIT:
            return "DHCP client/server is in initial state (not yet started)";

        case ESP_NETIF_DHCP_STOPPED:
            return "DHCP client/server has been stopped";

        case ESP_NETIF_DHCP_STARTED:
            return "DHCP client/server has been started";

        default:
            return "N/A";
    }
}


// For reasons I can't quite explain, I prefer a much longer polling
// period to reconnect to WiFi.  The idea of banging on a reconnect
// over and over as fast as you can feels wrong.
static void wifi_retry_callback(TimerHandle_t pxTimer)
{
    static const char* TAG = "wifi_retry_callback";

    if(last_wifi_event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "reconnect");

        // esp-idf notes that it's up to the programmer to know
        // when a disconnect() was called - that eminates a
        // WIFI_EVENT_STA_DISCONNECTED too
        esp_wifi_connect();
    }
    else
    {
        ESP_LOGD(TAG, "nominal");
    }
}

#if !FEATURE_WIFI_POLL_USE_RTOS_TIMER
void wifi_retry_poll()
{
    wifi_retry_callback({});
}
#endif

// EXPERIMENTAL, not used - the wifi_retry looks to be what we
// really need
/*
static void dhcp_retry_callback(TimerHandle_t pxTimer)
{
    static int counts_with_no_ip = 0;
    static const char* TAG = "dhcp_retry_callback";

    esp_netif_dhcp_status_t status;
    ESP_ERROR_CHECK(esp_netif_dhcpc_get_status(wifi_netif, &status));

    ESP_LOGD(TAG, "dhcpc status=%s", to_string(status));

    if(last_ip_event_id == IP_EVENT_STA_LOST_IP || got_ip == false)
    {
        ESP_LOGI(TAG, "restarting dhcpc");
        // NOTE: This causes stack overflow when performed inside timer
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(wifi_netif));
        ESP_ERROR_CHECK(esp_netif_dhcpc_start(wifi_netif));
    }
}

static estd::freertos::timer<true> dhcp_rety_timer(
    "DHCP retry",
    estd::chrono::seconds(60),
    //estd::chrono::seconds(600 * 5),
    true,
    nullptr,
    dhcp_retry_callback);
*/
#if FEATURE_WIFI_POLL_USE_RTOS_TIMER
static estd::freertos::timer<true> wifi_rety_timer(
    "WiFi retry",
    //estd::chrono::seconds(60),
    estd::chrono::seconds(WIFI_POLL_TIMEOUT_S),
    true,
    nullptr,
    wifi_retry_callback);
#endif


static constexpr estd::chrono::milliseconds timeout(50);

static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                got_ip = true;
                break;

            case IP_EVENT_STA_LOST_IP:
                got_ip = false;
                break;

            default:
                break;
        }

        last_ip_event_id = event_id;
    }
    else if(event_base == WIFI_EVENT)
    {
        last_wifi_event_id = event_id;
    }
}

void initialize_ip_retry()
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    //dhcp_rety_timer.reset(timeout);
    //dhcp_retry_callback(nullptr);
#if FEATURE_WIFI_POLL_USE_RTOS_TIMER
    wifi_rety_timer.start(timeout);
#endif
}
