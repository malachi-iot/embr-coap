#include "esp_log.h"

#include <estd/chrono.h>

#include <esp-helper.h>

#include <embr/platform/lwip/udp.h>
#include <coap/platform/ip.h>

#include <exp/retry/tracker.hpp>

#include "app.h"

static embr::lwip::udp::Pcb pcb;

void app_init(void** pcb_recv_arg, embr::lwip::udp::Pcb pcb)
{
    const char* TAG = "app_init";

    ESP_LOGD(TAG, "entry");

    ::pcb = pcb;
}

namespace app::lwip {

static void loop(duration t)
{
    const char* TAG = "app::lwip::loop";

    tracker_type::value_type* tracked = tracker.ready(t);

    if(tracked)
    {
        if(tracked->ack_received())
            tracker.mark_ack_processed();
        else
        {
            struct pbuf* p_ram = tracked->buffer();
            struct pbuf* p = pbuf_alloc(
                PBUF_TRANSPORT,
                0,
                PBUF_ROM);

            ESP_LOGI(TAG, "Retransmitting: phase 0 p_ram ref count=%u, tot_len=%u",
                p_ram->ref, p_ram->tot_len);
            pbuf_chain(p, p_ram);

            ESP_LOGI(TAG, "Retransmitting: phase 0.1 p_ram ref count=%u, tot_len=%u",
                p_ram->ref, p_ram->tot_len);

            ESP_LOGI(TAG, "Retransmitting: counter %u, ref count=%u, p=%p, tot_len=%u",
                tracked->retransmission_counter,
                p->ref,
                p,
                p->tot_len
                );

            err_t r = pcb.send_experimental(
                p,
                tracked->endpoint()
                );

            ESP_LOGI(TAG, "Retransmitting: phase 1 r=%d, p ref count=%u, pram ref count=%u",
                r, p->ref, p_ram->ref);

            pbuf_free(p);

            ESP_LOGI(TAG, "Retransmitting: phase 2 pram ref count=%u", p_ram->ref);

            bool processed = tracker.mark_con_sent();

            // FIX: Feels like a glitch in priority_queue's && treatment,
            // we're encountering some copy constructors (it seems) on our
            // Pbuf wrapper
            pbuf_free(p_ram);
            pbuf_free(p_ram);

            ESP_LOGI(TAG, "Retransmitting: processed=%u, pram ref_count=%u",
                processed,
                p_ram->ref);
        }
    }
}

}

namespace app {

void loop()
{
    const char* TAG = "app::loop";

    ESP_LOGI(TAG, "entry");

    for(;;)
    {
        vTaskDelay(10);
        auto now = estd::chrono::freertos_clock::now();
        auto t = now.time_since_epoch();
#if CONFIG_UDP
        app::lwip::loop(t);
#endif
#if CONFIG_ESP_NOW
        app::esp_now::loop(t);
#endif
    }
}

}