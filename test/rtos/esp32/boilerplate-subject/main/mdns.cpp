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

    // If not specific, mDNS pulls direct from LwIP/netif name (CONFIG_LWIP_LOCAL_HOSTNAME)
    // Specifically, "If not set, the hostname will be read from the interface"
    // As per https://docs.espressif.com/projects/esp-protocols/mdns/docs/1.2.1/en/index.html
    // However, that appears to not be the case.  mdns_hostname_set must be called, otherwise
    // nothing appears in avahi-browse
    mdns_hostname_set(CONFIG_LWIP_LOCAL_HOSTNAME);
    mdns_instance_name_set("CoAP boilerplate application");
    mdns_service_add("CoAP Boilerplate App", "_coap", "_udp", embr::coap::IP_PORT, NULL, 0);
}