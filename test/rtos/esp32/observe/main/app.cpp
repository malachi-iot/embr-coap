#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

#include <coap/decoder/observer/diagnostic.h>

#include "app.h"

using namespace embr::coap;

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


struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext,
    embr::coap::internal::ExtraContext
{
    //embr::coap::experimental::observable::option_value_type observe_option;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port) :
        embr::coap::LwipIncomingContext(pcb, addr, port),
        embr::coap::UriParserContext(paths::map)
    {}
};


struct ObservableObserver
{
    static void on_notify(const event::option& e, embr::coap::internal::ExtraContext& context)
    {
        // DEBT: Only accounts for request/GET mode - to be framework ready,
        // need to account also for notification receipt
        if(e.option_number == Option::Observe)
        {
            uint16_t value = UInt::get<uint16_t>(e.chunk);
            //context.observe_option = (experimental::observable::Options)value;
            context.flags.observable = (experimental::observable::Options)value;
        }
    }


    static Header::Code::Codes register_or_deregister(AppContext& context)
    {
        embr::coap::layer2::Token token(context.token());
        //endpoint_type endpoint(context.address().address(), IP_PORT);

        experimental::ObserveEndpointKey<endpoint_type>
            //key(endpoint, token);
            key(context.address(), token);

        int path_id = context.found_node();

        switch(context.observe_option().value())
        {
            case experimental::observable::Register:
                notifier2->registrar.add(key, path_id);
                return Header::Code::Valid;

            case experimental::observable::Deregister:
                // not ready yet
                //registrar.remove(key, path_id);
                return Header::Code::NotImplemented;

            default:
                return Header::Code::InternalServerError;
                break;
        }
    }
};


struct App
{
    void on_notify(event::completed, AppContext& context)
    {
        AppContext::encoder_type encoder = AppContext::encoder_factory::create();

        if(context.observe_option() && context.uri_matcher().last_found())
        {
            // DEBT: Filter out by GET and particular stat URI

            Header::Code::Codes code = 
                ObservableObserver::register_or_deregister(context);

            build_reply(context, encoder, code);

            if(code == Header::Code::Valid)
            {
                // DEBT: Need to lift actual current sequence number here
                build_stat(encoder, 0);
            }
            else
            {
                build_stat(encoder);
            }
        }
        else
        {
            switch(context.found_node())
            {
                case paths::v1_api_stats:
                    build_stat(encoder);
                    break;
                
                default:
                    build_reply(context, encoder, Header::Code::NotFound);
                    break;
            }
        }

        context.reply(encoder);
    }
};

embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver,
    UriParserObserver,
    ObservableObserver,
    embr::coap::internal::DiagnosticObserver,
    App
    > app_subject;


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    AppContext context(pcb, addr, port);

    decode_and_notify(p, app_subject, context);
}