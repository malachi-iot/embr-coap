#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <exp/lwip/subject.hpp>

#include <estd/port/freertos/timer.h>

#include "app.h"

using namespace embr::coap;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;
typedef experimental::observable::Registrar<endpoint_type> registrar_type;
typedef experimental::observable::lwip::Notifier notifier_type;
typedef notifier_type::encoder_type encoder_type;

registrar_type registrar;

static notifier_type notifier;
static embr::lwip::udp::Pcb notifying_pcb;

extern struct udp_pcb* pcb_hack;

void notifier_timer(void*)
{
    static const char* TAG = "notifier_timer";

    static unsigned sequence = 0;

    int count = registrar.observers.size();

    notifying_pcb = pcb_hack;

    ESP_LOGD(TAG, "entry: count=%d, sequence=%u", count, sequence);

    if(count > 0)
    {
        notifier.notify(registrar, paths::v1_api_stats, notifying_pcb,
            [](const registrar_type::key_type& key, encoder_type& encoder)
            {
                ESP_LOGD(TAG, "notify");

                encoder.option(Option::Observe, sequence);
                encoder.option(Option::ContentFormat, Option::TextPlain);
                encoder.payload();
                encoder_type::ostream_type out = encoder.ostream();

                out << "hi2u";
            });

        ++sequence;
    }
}

void app_init(void** pcb_recv_arg)
{
    static const char* TAG = "app_init";

    static estd::freertos::timer<true> timer("coap notifier",
        estd::chrono::seconds(5),
        true, nullptr,
        notifier_timer);

    //notifying_pcb.alloc();
    //notifying_pcb.bind(IP4_ADDR_ANY, IP_PORT);

    ESP_LOGI(TAG, "entry");

    timer.start(estd::chrono::seconds(5));
}