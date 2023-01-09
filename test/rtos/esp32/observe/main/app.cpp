#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/context.h>
#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/subject-core.hpp>

#include <coap/platform/lwip/rfc7641/notifier.hpp>

#include <coap/decoder/observer/core.h>
#include <coap/decoder/observer/diagnostic.h>

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
    embr::coap::UriParserContext
{
    //embr::coap::experimental::observable::option_value_type observe_option;

    registrar_type& registrar;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port,
        Notifier* notifier) :
        embr::coap::LwipIncomingContext(pcb, addr, port),
        embr::coap::UriParserContext(paths::map),
        registrar(notifier->registrar())
    {}
};



struct App
{
    static void build_stat_with_observe(AppContext& context, AppContext::encoder_type& encoder)
    {
        static const char* TAG = "build_stat_with_observe";

        typedef internal::observable::RegistrarTraits<registrar_type> traits;

        Header::Code added_or_removed = add_or_remove(
            context.registrar,
            context, 
            context.observe_option(), paths::v1_stats);

        ESP_LOGD(TAG, "added_or_removed=%u", get_http_style(added_or_removed));

        build_reply(context, encoder, Header::Code::Valid);

        uint32_t sequence = added_or_removed.success() ?
            // Will be '0' or '1', indicating a successful register or deregister
            context.observe_option() :
            // 'failure' just means we fall back to a regular non-observe way
            // DEBT: Depends on singleton API.  Not so bad, just be aware
            traits::sequence(context.registrar);

        build_stat_suffix(encoder, sequence);
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
                context.response_code = nvs_load_registrar(
                    &context.registrar);
                break;

            case paths::v1_save:
                nvs_save_registrar(&context.registrar);
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
    AppContext context(pcb, addr, port, (Notifier *)notifier);

    decode_and_notify(p, app_subject, context);
}