#include "esp_log.h"

#include <esp-helper.h>


#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/udp.h>

// DEBT: At the moment, this is where StreambufProvider specialization lives
#include <coap/platform/lwip/factory.h>
#include <coap/platform/ip.h>

#include <coap/header.h>

#include "app.h"


void udp_coap_recv(void *arg, 
    struct udp_pcb *_pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    static const char* TAG = "udp_coap_receive";

    ESP_LOGI(TAG, "entry");

    embr::lwip::udp::Pcb pcb(_pcb);
    embr::lwip::Pbuf pbuf(p);

    embr::coap::Header header = embr::coap::experimental::get_header(pbuf);

    ESP_LOGD(TAG, "mid=%x", header.message_id());

#if FEATURE_RETRY_MANAGER
    auto manager = (manager_type*) arg;
#endif
#if FEATURE_RETRY_TRACKER_V2
#endif
}