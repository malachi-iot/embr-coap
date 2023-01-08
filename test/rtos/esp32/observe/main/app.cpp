#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <coap/platform/lwip/rfc7641/notifier.hpp>

#include <coap/decoder/observer/uri.h>         // For UriPathMap
#include <coap/decoder/observer/util.h>
#include <coap/decoder/observer/core.h>
#include <coap/decoder/observer/diagnostic.h>
#include <coap/decoder/observer/observable.h>

#include "app.h"
#include "notifier.h"                       // For semi-experimental NotifierContext

using namespace embr::coap;

using embr::coap::internal::UriPathMap;

namespace paths {

// NOTE: Alphabetization per path segment is important.  id# ordering is not
// DEBT: Document this behavior in detail
const UriPathMap map[] =
{
    { "v1",         v1,                 MCCOAP_URIPATH_NONE },
    { "stats",      v1_stats,           v1 },
    { "load",       v1_load,            v1 },
    { "save",       v1_save,            v1 }
};


}



struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext,
    NotifierContext<registrar_type>
{
    //embr::coap::experimental::observable::option_value_type observe_option;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port,
        Notifier& notifier) :
        embr::coap::LwipIncomingContext(pcb, addr, port),
        embr::coap::UriParserContext(paths::map),
        NotifierContext<registrar_type>(notifier)
    {}
};



struct App
{
    static void build_stat_with_observe(AppContext& context, AppContext::encoder_type& encoder)
    {
        static const char* TAG = "build_stat_with_observe";

        Header::Code added_or_removed = add_or_remove(
            context.notifier().registrar(),
            context, 
            context.observe_option(), paths::v1_stats);

        ESP_LOGD(TAG, "added_or_removed=%u", get_http_style(added_or_removed));

        build_reply(context, encoder, Header::Code::Valid);

        if(added_or_removed.success())
            // Will be '0' or '1', indicating a successful register or deregister
            build_stat_suffix(encoder, context.observe_option());
        else
            // build regular non-observed response
            // DEBT: Need to lift actual current sequence number here
            build_stat_suffix(encoder);

        context.reply(encoder);
    }

    static void on_notify(event::completed, AppContext& context)
    {
        AppContext::encoder_type encoder = AppContext::encoder_factory::create();

        switch(context.found_node())
        {
            case paths::v1_stats:
                if(verify(context, Header::Code::Get))
                    build_stat_with_observe(context, encoder);

                break;

            case paths::v1_load:
                context.response_code = nvs_load_registrar();
                break;

            case paths::v1_save:
                nvs_save_registrar();
                context.response_code = Header::Code::Changed;
                break;
            
            default:
                break;
        }

        auto_reply(context, encoder);
    }
};

embr::layer0::subject<
    CoreObserver,
    embr::coap::internal::DiagnosticObserver,
    App
    > app_subject;


void udp_coap_recv(void* notifier, 
    struct udp_pcb* pcb, struct pbuf* p,
    const ip_addr_t* addr, u16_t port)
{
    AppContext context(pcb, addr, port, *(Notifier *)notifier);

    decode_and_notify(p, app_subject, context);
}