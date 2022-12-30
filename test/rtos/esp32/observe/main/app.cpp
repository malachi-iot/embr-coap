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


namespace tags2 {

struct notifier_context {};

}

template <class TRegistrar>
struct NotifierContext : tags2::notifier_context
{
    typedef ::internal::NotifyHelperBase<TRegistrar> notifier_type;

    notifier_type& notifier_;

    notifier_type& notifier() const { return notifier_; }

    NotifierContext(notifier_type& n) : notifier_(n) {}
};


namespace layer0 {

template <class TRegistrar>
struct NotifierContext
{
    typedef ::internal::NotifyHelperBase<TRegistrar> notifier_type;

    notifier_type& notifier_;
};

}


struct AppContext : 
    embr::coap::LwipIncomingContext,
    embr::coap::UriParserContext,
    embr::coap::internal::ExtraContext,
    NotifierContext<registrar_type>
{
    //embr::coap::experimental::observable::option_value_type observe_option;

    AppContext(pcb_pointer pcb,
        const ip_addr_t* addr,
        uint16_t port) :
        embr::coap::LwipIncomingContext(pcb, addr, port),
        embr::coap::UriParserContext(paths::map),
        NotifierContext<registrar_type>(*::notifier)
    {}
};

template <class TRegistrar, class TContext, typename enable =
    typename estd::enable_if<
        estd::is_base_of<tags::token_context, TContext>::value &&
        estd::is_base_of<tags::address_context, TContext>::value
    >::type
>
bool add_or_remove(
    ::internal::NotifyHelperBase<TRegistrar>& notifier,
    TContext& context,
    embr::coap::experimental::observable::option_value_type option_value,
    int resource_id)
{
    Header::Code::Codes code = Header::Code::NotImplemented;

    code = notifier.add_or_remove(
        option_value.value(),
        context.address(),
        context.token(),
        resource_id);

    return code == Header::Code::Created;
}


struct ObservableObserver
{
    static void on_notify(const event::option& e, embr::coap::internal::ExtraContext& context)
    {
        // DEBT: Only accounts for request/GET mode - to be framework ready,
        // need to account also for notification receipt (i.e. sequence number)
        if(e.option_number == Option::Observe)
        {
            uint16_t value = UInt::get<uint16_t>(e.chunk);
            context.flags.observable = (experimental::observable::Options)value;
        }
    }

// Really nifty code, but alas we don't want to observe EVERY request
#if UNUSED
    template <class TContext, typename enable =
        typename estd::enable_if<
            estd::is_base_of<tags::token_context, TContext>::value &&
            estd::is_base_of<tags::address_context, TContext>::value &&
            estd::is_base_of<embr::coap::internal::ExtraContext, TContext>::value &&
            estd::is_base_of<tags2::notifier_context, TContext>::value
        >::type
    >
    static void on_notify(event::option_completed, TContext& context)
    {
        Header::Code::Codes code = Header::Code::NotImplemented;

        code = context.notifier().add_or_remove(
            context.observe_option().value(),
            context.address(),
            context.token(),
            context.found_node());

        if(code != Header::Code::Valid)
        {
            context.flags.observable = experimental::observable::Unspecified;
        }
    }
#endif
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
        bool added_or_removed = add_or_remove(*notifier, context, 
            context.observe_option(), paths::v1_api_stats);
        
        if(added_or_removed)
            // DEBT: Need to lift actual current sequence number here
            build_stat(context, encoder, Header::Code::Valid, 0);
        else
            // build regular non-observed response
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
                else
                    build_stat_with_observe(context, encoder);
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