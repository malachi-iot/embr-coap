// TODO: esp-idf v5 needs extra dependency management
#include "mdns.h"

#include "esp_log.h"

#include <coap/platform/ip.h>

void initialize_mdns()
{
    static const char* TAG = "initialize_mdns";

    esp_err_t err = mdns_init();

    if (err)
    {
        ESP_LOGW(TAG, "mDNS Init failed: %d", err);
        return;
    }

    mdns_hostname_set("coap-boilerplate");
    mdns_instance_name_set("CoAP boilerplate application");
    mdns_service_add("CoAP Boilerplate App", "_coap", "_udp", embr::coap::IP_PORT, NULL, 0);
}