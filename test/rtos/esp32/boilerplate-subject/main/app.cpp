#include <estd/istream.h>

#include <embr/observer.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace embr::coap;

#include "context.h"

#ifdef ESP_PLATFORM
// This gets us 'build_version_response' which is indeed esp-idf specific
#include <coap/platform/esp-idf/observer.h>
#define FEATURE_APP_GPIO 1
#else
#endif

#include <coap/platform/lwip/encoder.h>

using istreambuf_type = AppContext::istreambuf_type;

enum
{
    id_path_v1 = 0,
    id_path_v1_api,
    id_path_v1_api_gpio,
    id_path_v1_api_version,
    id_path_v1_api_stats,
    id_path_v1_api_gpio_value,

    id_path_well_known,
    id_path_well_known_core
};


using embr::coap::internal::UriPathMap;




// NOTE: Alphabetization per path segment is important.  id# ordering is not
// DEBT: Document this behavior in detail
const UriPathMap uri_map[] =
{
    { "v1",         id_path_v1,                 MCCOAP_URIPATH_NONE },
    { "api",        id_path_v1_api,             id_path_v1 },
    { "gpio",       id_path_v1_api_gpio,        id_path_v1_api },
    { "*",          id_path_v1_api_gpio_value,  id_path_v1_api_gpio },
    { "stats",      id_path_v1_api_stats,       id_path_v1_api },
    
    { "sys",        sys_paths::v1::root,        id_path_v1 },
    { "version",    sys_paths::v1::root_version,        sys_paths::v1::root },
    
    { "version",    id_path_v1_api_version,     id_path_v1_api },

    { ".well-known",    id_path_well_known,         MCCOAP_URIPATH_NONE },
    { "core",           id_path_well_known_core,    id_path_well_known }
};




struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(const event::option& e, AppContext& context)
    {
        if(context.found_node() == id_path_v1_api_gpio_value)
            context.select_gpio(e);
    }

    static void on_notify(event::streambuf_payload<istreambuf_type> e, AppContext& context)
    {
        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
                context.put_gpio(e.streambuf);
                break;
        }
    }

    
    __attribute__ ((noinline))  // Not necessary, just useful for stack usage analysis
    static void on_notify(event::completed, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        auto encoder = make_encoder(context);

        switch(context.found_node())
        {
            case id_path_v1_api_gpio:
                build_reply(context, encoder, Header::Code::NotImplemented);
                break;
                
            case id_path_v1_api_gpio_value:
                context.completed_gpio(encoder);
                break;
                
#ifdef ESP_PLATFORM
            case id_path_v1_api_version:
                build_app_version_response(context, encoder);
                break;
#endif

            default:
                if(!sys_paths::build_sys_reply(context, encoder))
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
    Observer
    > app_subject;


AppContext::AppContext(struct udp_pcb* pcb, 
    const ip_addr_t* addr,
    uint16_t port) : 
    LwipIncomingContext(pcb, addr, port),
    UriParserContext(uri_map)
{

}


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    // Because AppContext is on the stack and app_subject is layer0 (stateless)
    // This code is, in theory, reentrant.  That has not been tested well, however
    AppContext context(pcb, addr, port);

    ESP_LOGD(TAG, "p->len=%d, sizeof context=%u", p->len, sizeof(context));

    // _recv plumbing depends on us to free p, and decode_and_notify here helps us with that
    // DEBT: I think I prefer explicitly freeing here, decode_and_notify assistance is too magic
    decode_and_notify(p, app_subject, context);
}


#if FEATURE_APP_GPIO == 0
void AppContext::select_gpio(const event::option& e) {}
void AppContext::put_gpio(istreambuf_type& streambuf) {}
void AppContext::completed_gpio(encoder_type& encoder)
{
    build_reply(*this, encoder, Header::Code::NotImplemented);
}
#endif