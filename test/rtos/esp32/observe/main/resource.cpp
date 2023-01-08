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


// FIX: Works for a little while, but then crash/restart occurs
Header::Code nvs_load_registrar(registrar_type* r)
{
    static const char* TAG = "nvs_load_registrar";

    embr::internal::scoped_guard<embr::esp_idf::nvs::Handle>
        h(STORAGE_NAMESPACE, NVS_READWRITE);

    std::size_t sz;

    ESP_ERROR_CHECK(h->get_blob(nvs_reg_key, r, &sz));

    ESP_LOGI(TAG, "loaded: sz=%u / sizeof=%u", sz, sizeof(registrar_type));

    if(sz != sizeof(registrar_type))
    {
        ESP_LOGE(TAG, "uh oh!  load had a problem, sizes don't match");
        return Header::Code::InternalServerError;
    }

    return Header::Code::Valid;
}

void nvs_save_registrar(registrar_type* r)
{
    static const char* TAG = "nvs_save_registrar";

    nvs_handle_t my_handle;
    //esp_err_t err;
    
    ESP_ERROR_CHECK(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle));

    ESP_ERROR_CHECK(nvs_set_blob(my_handle, nvs_reg_key,
        r, sizeof(registrar_type)));

    ESP_LOGI(TAG, "saved");

    nvs_close(my_handle);
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


static void notifier_timer(TimerHandle_t)
{
    static const char* TAG = "notifier_timer";

    registrar_type& r = notifier->registrar();
    int count = r.observer_count();

    // DEBT: Way too invasive
    uint32_t& sequence = r.sequence;

    ESP_LOGD(TAG, "entry: count=%d, sequence=%" PRIu32, count, sequence);

    if(count > 0)
    {
        notifier->notify(paths::v1_stats,
            [=](const registrar_type::key_type& key, encoder_type& encoder)
            {
                ESP_LOGD(TAG, "notify");

                build_stat_suffix(encoder, sequence);
            });

        ++sequence;
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

    *pcb_recv_arg = notifier = &nh;
}
