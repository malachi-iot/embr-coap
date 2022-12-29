#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

using namespace embr::coap;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

extern experimental::observable::Registrar<endpoint_type> registrar;

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
    estd::monostate context;

    decode_and_notify(p, app_subject, context);
}