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
            context.flags.observable = (experimental::observable::Options)value;
        }
    }
};


// build_stat with header built also
static void build_stat(AppContext& context, AppContext::encoder_type& encoder,
    Header::Code::Codes code,
    sequence_type sequence = sequence_type())
{
    build_reply(context, encoder, code);
    build_stat(encoder, sequence);
}


struct App
{
    static void build_stat_with_observe(AppContext& context, AppContext::encoder_type& encoder)
    {
        Header::Code::Codes code = notifier->add_or_remove(
            context.observe_option().value(),
            context.address(),
            context.token(),
            context.found_node());

        if(code == Header::Code::Valid)
            // DEBT: Need to lift actual current sequence number here
            build_stat(context, encoder, code, 0);
        else
            // if status code for deducing is not successful, build regular non-observed
            // response
            build_stat(context, encoder, Header::Code::Valid);
    }

    static void on_notify(event::completed, AppContext& context)
    {
        AppContext::encoder_type encoder = AppContext::encoder_factory::create();

        switch(context.found_node())
        {
            case paths::v1_api_stats:
                if(context.header().code() != Header::Code::Get)
                    build_reply(context, encoder, Header::Code::BadRequest);
                else if(context.observe_option())
                    build_stat_with_observe(context, encoder);
                else
                    build_stat(context, encoder, Header::Code::Valid);
                break;
            
            default:
                build_reply(context, encoder, Header::Code::NotFound);
                break;
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