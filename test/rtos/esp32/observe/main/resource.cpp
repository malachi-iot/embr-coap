#include "esp_log.h"

#include <embr/observer.h>

#include <coap/platform/lwip/encoder.h>
#include <coap/decoder.hpp>
#include <coap/decoder/events.h>
#include <coap/decoder/subject-core.hpp>

#include <estd/port/freertos/timer.h>

#include "app.h"

using namespace embr::coap;


NotifyHelper* notifier;


void build_stat(encoder_type& encoder, sequence_type sequence)
{
    if(sequence)
        encoder.option(Option::Observe, sequence.value());

    encoder.option(Option::ContentFormat, Option::TextPlain);
    encoder.payload();
    encoder_type::ostream_type out = encoder.ostream();

    out << "hi2u: " << sequence.value();
}


void notifier_timer(void*)
{
    static const char* TAG = "notifier_timer";

    // DEBT: Emitting a observe sequence of 0 seems to agitate aiocoap - perhaps it tosses it out
    // because initial response has an observe '0'?
    static unsigned sequence = 1;

    int count = notifier->observer_count();

    ESP_LOGD(TAG, "entry: count=%d, sequence=%u", count, sequence);

    if(count > 0)
    {
        notifier->notify(paths::v1_api_stats,
            [](const registrar_type::key_type& key, encoder_type& encoder)
            {
                ESP_LOGD(TAG, "notify");

                build_stat(encoder, sequence);
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




void app_init(embr::lwip::udp::Pcb pcb)
{
    // Tricky, taking advantage of a C++ behavior - seems to work.  app_init
    // is only ever run once.  Calls constructor correctly during app_init phase
    // DEBT: I wonder what would happen if app_init were run twice?
    static NotifyHelper nh(pcb);

    notifier = &nh;
}