#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

#include "app.h"

using namespace embr::coap;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

extern experimental::observable::Registrar<endpoint_type> registrar;

using embr::coap::internal::UriPathMap;

namespace paths {

// NOTE: Alphabetization per path segment is important.  id# ordering is not
// DEBT: Document this behavior in detail
const UriPathMap map[] =
{
    { "v1",         v1,                 MCCOAP_URIPATH_NONE },
    { "api",        v1_api,             v1 },
    { "gpio",       v1_api_gpio,        v1_api },
    { "*",          v1_api_gpio_value,  v1_api_gpio },
    { "stats",      v1_api_stats,       v1_api },
    { "version",    v1_api_version,     v1_api },

    { ".well-known",    well_known,         MCCOAP_URIPATH_NONE },
    { "core",           well_known_core,    well_known }
};


}


enum ObserveOptions
{
    Unspecified = 100,
    Register = 0,
    Deregister = 1
};


struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext
{
    estd::layer1::optional<ObserveOptions, Unspecified> observe_option;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port) :
        embr::coap::LwipIncomingContext(pcb, addr, port),
        embr::coap::UriParserContext(paths::map)
    {}
};

struct App
{
    void on_notify(const event::option& e, AppContext& context)
    {
        if(e.option_number == Option::Observe)
        {
            context.observe_option = (ObserveOptions)UInt::get<uint16_t>(e.chunk);
        }
    }


    void on_notify(event::completed, AppContext& context)
    {
        AppContext::encoder_type encoder = AppContext::encoder_factory::create();

        if(context.observe_option && context.uri_matcher().last_found())
        {
            embr::coap::layer2::Token token(context.token());
            endpoint_type endpoint(context.address().address(), IP_PORT);
            
            experimental::ObserveEndpointKey<endpoint_type> key(endpoint, token);

            Header::Code::Codes code;
            int path_id = context.found_node();

            switch(context.observe_option.value())
            {
                case Register:
                    registrar.add(key, path_id);
                    code = Header::Code::Valid;
                    break;

                case Deregister:
                    // not ready yet
                    //registrar.remove(key, path_id);
                    code = Header::Code::NotImplemented;
                    break;

                default:
                    code = Header::Code::InternalServerError;
                    break;
            }

            build_reply(context, encoder, code);

            encoder.option(Option::Observe);
        }
        else
            build_reply(context, encoder, Header::Code::NotFound);

        context.reply(encoder);
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