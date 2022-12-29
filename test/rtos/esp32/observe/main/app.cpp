#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

using namespace embr::coap;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

extern experimental::observable::Registrar<endpoint_type> registrar;

struct AppContext : 
    embr::coap::LwipIncomingContext
{
    typedef embr::coap::LwipContext::lwip_encoder_factory<8> encoder_factory;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port) :
        embr::coap::LwipIncomingContext(pcb, addr, port)
    {}
};

struct App
{
    void on_notify(event::option e)
    {
        if(e.option_number == Option::Observe)
        {
            //endpoint_type endpoint(e.)
            //experimental::ObserveEndpointKey<endpoint_type> key()
        }
    }


    void on_notify(event::completed, AppContext& context)
    {
    }
};

embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver,
    UriParserObserver,
    App
    > app_subject;


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    AppContext context(pcb, addr, port);

    decode_and_notify(p, app_subject, context);
}