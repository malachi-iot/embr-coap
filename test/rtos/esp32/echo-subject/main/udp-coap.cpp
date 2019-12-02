#include "esp_log.h"

#include <embr/platform/lwip/streambuf.h>
#include <embr/streambuf.h>

#include <embr/observer.h>
// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include <coap/header.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

// TODO: Time to clean exp events up, functionality is not
// experimental any longer
#include <exp/events.h>


#define COAP_UDP_PORT 5683

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace embr::coap::experimental;

#include "observer-util.h"
#include "context.h"


struct Observer
{
    static constexpr const char* TAG = "Observer";

    void on_notify(completed_event, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        AppContext::encoder_type encoder = context.make_encoder();

        Header header = context.header();
        moducom::coap::layer3::Token token = context.token();

        header.type(Header::TypeEnum::Acknowledgement);
        header.code(Header::Code::Valid);

        encoder.header(header);
        encoder.token(token);
        encoder.finalize();

        context.reply(encoder);
    }
};

embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver> app_subject;

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    if (p != NULL)
    {
        ESP_LOGD(TAG, "p->len=%d", p->len);

        // TODO: Need an rvalue (&&) flavor of decode_and_notify so we can use a temp
        // subject
        auto subject = embr::layer1::make_subject(
            embr::layer1::make_observer_proxy(app_subject),
            Observer()
            );

        AppContext context(pcb, addr, port);

        context.do_notify(p, subject);
    }
}


void udp_coap_init(void)
{
    struct udp_pcb * pcb;

    /* get new pcb */
    pcb = udp_new();
    if (pcb == NULL) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return;
    }

    /* bind to any IP address */
    if (udp_bind(pcb, IP_ADDR_ANY, COAP_UDP_PORT) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    udp_recv(pcb, udp_coap_recv, NULL);
}