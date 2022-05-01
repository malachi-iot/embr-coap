// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace embr::coap;

#include "context.h"
#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

using embr::lwip::upgrading::ipbuf_streambuf;

constexpr int id_path_v1 = 0;
constexpr int id_path_v1_api = 1;
constexpr int id_path_v1_api_test = 3;
constexpr int id_path_v1_api_version = 4;
constexpr int id_path_v1_api_stats = 5;
constexpr int id_path_well_known = 20;
constexpr int id_path_well_known_core = 21;

// NOTE: Alphabetization is important.  id# ordering is not
const UriPathMap uri_map[] =
{
    { "v1",         id_path_v1,                 MCCOAP_URIPATH_NONE },
    { "api",        id_path_v1_api,             id_path_v1 },
    { "stats",      id_path_v1_api_stats,       id_path_v1_api },
    { "test",       id_path_v1_api_test,        id_path_v1_api },
    { "version",    id_path_v1_api_version,     id_path_v1_api },

    { ".well-known",    id_path_well_known,         MCCOAP_URIPATH_NONE },
    { "core",           id_path_well_known_core,    id_path_well_known }
};


struct Observer
{
    static constexpr const char* TAG = "Observer";

    static void on_notify(event::completed, AppContext& context)
    {
        ESP_LOGI(TAG, "on_notify completed");

        auto encoder = make_encoder(context);

        switch(context.found_node())
        {
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


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    ESP_LOGD(TAG, "p->len=%d", p->len);

    // Because AppContext is on the stack and app_subject is layer0 (stateless)
    // This code is, in theory, reentrant.  That has not been tested well, however
    AppContext context(pcb, addr, port);

    // _recv plumbing depends on us to frees p,
    decode_and_notify(p, app_subject, context);
}
