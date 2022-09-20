#include "esp_log.h"

#include <esp-helper.h>

#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/udp.h>

#include <coap/platform/ip.h>

#include <coap/header.h>

#include "app.h"

// DEBT: Need to consolidate this into coap/platform/lwip area once
// experimentation settles down
namespace embr { namespace coap { namespace experimental {

template <>
struct StreambufProvider<embr::lwip::Pbuf>
{
    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;
};

}}}


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

    auto manager = (manager_type*) arg;
}