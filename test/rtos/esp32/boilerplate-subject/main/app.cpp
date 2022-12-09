#include <estd/istream.h>

#include <embr/platform/esp-idf/gpio.h>

// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>
#include <coap/decoder/streambuf.hpp>

using namespace embr::coap;

#include "context.h"

// This gets us 'build_version_response' which is indeed esp-idf specific
#include <coap/platform/esp-idf/observer.h>

#include <coap/platform/lwip/encoder.h>

using embr::lwip::upgrading::ipbuf_streambuf;

constexpr int id_path_v1 = 0;
constexpr int id_path_v1_api = 1;
constexpr int id_path_v1_api_gpio = 3;
constexpr int id_path_v1_api_version = 4;
constexpr int id_path_v1_api_stats = 5;
constexpr int id_path_v1_api_gpio_value = 6;
constexpr int id_path_well_known = 20;
constexpr int id_path_well_known_core = 21;

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
        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
            {
                int& pin = context.gpio.pin;

                // DEBT: As is the case all over, 'chunk' is assumed to be complete
                // data here
                auto option = (const char*) e.chunk.data();
                
                if(estd::from_chars<int>(option, option + e.chunk.size(), pin).ec == 0)
                    ESP_LOGD(TAG, "Selecting gpio # %d", pin);
                else
                    pin = -1;

                break;
            }
        }
    }

    static void on_notify(event::streambuf_payload<ipbuf_streambuf> e, AppContext& context)
    {
        switch(context.found_node())
        {
            case id_path_v1_api_gpio_value:
            {
                if(context.gpio.pin == -1) break;

                if(context.header().code() == Header::Code::Put)
                {
                    //auto& s = e.streambuf;
                    // DEBT: I think this can be promoted out of internal, I believe I put
                    // this in there way back when because the signature doesn't match std - but
                    // by this point, that's a feature not a bug
                    estd::internal::basic_istream<ipbuf_streambuf&> in(e.streambuf);

                    int val = -1;

                    in >> val;

                    ESP_LOGI(TAG, "gpio: set #%d to %d", context.gpio.pin, val);

                    embr::esp_idf::gpio gpio((gpio_num_t)context.gpio.pin);

                    gpio.set_direction(GPIO_MODE_OUTPUT);
                    gpio.level(val);
                }
                else
                    ESP_LOGW(TAG, "gpio: undefined behavior - payload present, but not a put");

                break;
            }
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
            {
                if(context.header().code() == Header::Code::Get)
                {
                    ESP_LOGI(TAG, "gpio: get %d", context.gpio.pin);

                    embr::esp_idf::gpio gpio((gpio_num_t)context.gpio.pin);

                    int val = gpio.level();

                    build_reply(context, encoder, Header::Code::Content);

                    encoder.option(
                        Option::Numbers::ContentFormat,
                        Option::ContentFormats::TextPlain);

                    encoder.payload();

                    auto out = encoder.ostream();

                    out << val;
                }
                else if(context.header().code() == Header::Code::Put && context.gpio.pin != -1)
                    build_reply(context, encoder, Header::Code::Valid);
                else
                    build_reply(context, encoder, Header::Code::BadRequest);

                break;
            }
                
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

    // Because AppContext is on the stack and app_subject is layer0 (stateless)
    // This code is, in theory, reentrant.  That has not been tested well, however
    AppContext context(pcb, addr, port);

    ESP_LOGD(TAG, "p->len=%d, sizeof context=%u", p->len, sizeof(context));

    // _recv plumbing depends on us to free p,
    decode_and_notify(p, app_subject, context);
}
