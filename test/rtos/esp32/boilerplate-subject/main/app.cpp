#include <estd/istream.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace embr::coap;

#include "context.h"

// This gets us 'build_version_response' which is indeed esp-idf specific
#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

using embr::lwip::ipbuf_streambuf;

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

    static void on_notify(event::streambuf_payload<ipbuf_streambuf> e, AppContext& context)
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
                context.get_gpio(encoder);
                break;
                
            case id_path_v1_api_version:
                build_version_response(context, encoder);
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

    // _recv plumbing depends on us to free p,
    decode_and_notify(p, app_subject, context);
}
