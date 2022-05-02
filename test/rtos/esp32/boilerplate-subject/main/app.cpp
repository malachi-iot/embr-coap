#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

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


extern EventGroupHandle_t xEventGroupHandle;

embr::layer0::subject<
    HeaderContextObserver,
    TokenContextObserver,
    UriParserObserver,
    Observer
    > app_subject;

struct LwipState
{
    //embr::lwip::udp::Pcb pcb;
    struct udp_pcb* pcb;
    struct pbuf* pbuf;
    embr::lwip::experimental::Endpoint<> endpoint;

    LwipState() :
        endpoint(nullptr, 0)
    {}

    LwipState(
        //embr::lwip::udp::Pcb pcb,
        struct udp_pcb* pcb,
        struct pbuf* pbuf,
        const ip_addr_t* addr, u16_t port) :
        pcb(pcb),
        pbuf(pbuf),
        endpoint(addr, port)
    {}
};

// fake 1-item queue
static LwipState state;


void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    new (&state) LwipState(pcb, p, addr, port);

    // TODO: PCB/raw API is finicky about stack size.  Prepping to flag an event
    // and handle pcb & pbuf off in app_main
    xEventGroupSetBits(xEventGroupHandle, 1);

    pbuf_ref(p);
}


void udp_coap_recv_freertos()
{
    const char* TAG = "udp_coap_recv_freertos";

    // Because AppContext is on the stack and app_subject is layer0 (stateless)
    // This code is, in theory, reentrant.  That has not been tested well, however
    AppContext context(state.pcb, 
        state.endpoint.address(), state.endpoint.port());

    // 01MAY22 context size reports 40
    ESP_LOGD(TAG, "p->len=%d, sizeof context=%u", state.pbuf->len, sizeof(context));

    // _recv plumbing depends on us to frees p,
    decode_and_notify(state.pbuf, app_subject, context);
}