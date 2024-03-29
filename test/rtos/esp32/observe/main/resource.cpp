#include "esp_log.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <embr/observer.h>


// DEBT: tricky to know this is necessary for finalize()
#include <coap/platform/lwip/encoder.h>

#include <coap/decoder.hpp>
#include <coap/decoder/subject-core.hpp>

#include <estd/port/freertos/timer.h>

#include "app.h"
#include "fancy.hpp"

using namespace embr::coap;


static Notifier* notifier;

static const char* nvs_reg_key = "coap::reg";
static const char* nvs_seq_key = "coap::seq";   // DEBT: Not yet implemented

#define STORAGE_NAMESPACE "storage"


Header::Code nvs_load_registrar(registrar_type* r)
{
    static const char* TAG = "nvs_load_registrar";

    if(embr::esp_idf::nvs::get(STORAGE_NAMESPACE, nvs_reg_key, r) != ESP_OK)
        return Header::Code::InternalServerError;
    else
    {
        ESP_LOGI(TAG, "loaded: sizeof=%u", sizeof(registrar_type));
        return Header::Code::Valid;
    }
}

void nvs_save_registrar(registrar_type* r)
{
    static const char* TAG = "nvs_save_registrar";

    ESP_ERROR_CHECK(embr::esp_idf::nvs::set(STORAGE_NAMESPACE, nvs_reg_key, r));

    ESP_LOGI(TAG, "saved");
}


void build_stat_suffix(encoder_type& encoder, sequence_type sequence)
{
    if(sequence)
        encoder.option(Option::Observe, sequence.value());

    encoder.option(Option::ContentFormat, Option::TextPlain);
    encoder.payload();
    encoder_type::ostream_type out = encoder.ostream();

    out << "hi2u: " << sequence.value();
}


#if CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH < 3000
#warning "Likely you will experience stack overflows here"
#endif
static void notifier_timer(TimerHandle_t)
{
    static const char* TAG = "notifier_timer";

    typedef internal::observable::RegistrarTraits<registrar_type> traits;

    registrar_type& r = notifier->registrar();
    int count = r.observer_count();

    // NOTE: Only really checking compile time behavior of RefNotifier.
    // Leaning non non-ref version for runtime test
    RefNotifier rn(notifier->pcb, r);

    ESP_LOGD(TAG, "entry: count=%d", count);

    if(count > 0)
    {
        static int toggle = 0;

        if(++toggle % 2 == 0)
        {
            // Notify via layer1 style
            notifier->notify(paths::v1_stats,
                [&](const registrar_type::key_type& key, encoder_type& encoder)
                {
                    uint32_t sequence = traits::sequence(r, key);

                    ESP_LOGI(TAG, "notify: %s, sequence=%" PRIu32,
                        ipaddr_ntoa(key.endpoint.address()), sequence);
                    ESP_LOG_BUFFER_HEXDUMP(TAG, key.token.data(), key.token.size(), ESP_LOG_DEBUG);

                    build_stat_suffix(encoder, sequence);
                });
        }
        else
            // Notify via layer2/layer3 style
            rn.notify(paths::v1_stats,
                [&](const registrar_type::key_type& key, encoder_type& encoder)
                {
                    uint32_t sequence = traits::sequence(r, key);

                    ESP_LOGI(TAG, "rn.notify: %s", ipaddr_ntoa(key.endpoint.address()));

                    build_stat_suffix(encoder, sequence);
                });

        // NOTE: Mixing and matching sequence tracking approaches just for
        // demonstration.  It's recommended to use the (r, key) signature
        traits::increment_sequence(r);
    }
}

void app_init(void** pcb_recv_arg)
{
    static const char* TAG = "app_init";

    // DEBT: Assign id to notifier, and we can then wipe out the global entirely!
    static estd::freertos::timer<true> timer("coap notifier",
        estd::chrono::seconds(5),
        true, nullptr,
        notifier_timer);

    ESP_LOGI(TAG, "entry");

    timer.start(estd::chrono::seconds(5));
}




void app_init(void** pcb_recv_arg, embr::lwip::udp::Pcb pcb)
{
    //static const char* TAG = "app_init";
    
    // Tricky, taking advantage of a C++ behavior - seems to work.  app_init
    // is only ever run once.  Calls constructor correctly during app_init phase
    // DEBT: I wonder what would happen if app_init were run twice?
    static Notifier nh(pcb);
    static RefNotifier rn(pcb, nh.registrar());

    *pcb_recv_arg = notifier = &nh;
}
