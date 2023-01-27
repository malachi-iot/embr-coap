#include "esp_log.h"

#include <embr/observer.h>
// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>

#include <estd/string.h>

#include <coap/header.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/decoder/observer/util.h>
#include <coap/encoder/streambuf.h>
#include <coap/platform/lwip/encoder.h>


using namespace embr::coap;

#include "context.h"    // lwip magic happens in here


struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(event::completed, AppContext& context)
    {
        ESP_LOGD(TAG, "on_notify completed");

        auto encoder = AppContext::encoder_factory::create();

        //AppContext::encoder_type encoder = make_encoder_reply(context, Header::Code::Valid);

        build_reply(context, encoder, Header::Code::Valid);

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

        decode_and_notify(p, subject, context);
    }
    else
        ESP_LOGW(TAG, "No pbuf");
}
