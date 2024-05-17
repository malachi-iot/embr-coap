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


void app_loop()
{
    //using time_point = tracker_type::time_point;
    const char* TAG = "app_loop";

    ESP_LOGI(TAG, "entry");

    for(;;)
    {
        vTaskDelay(10);
        auto now = estd::chrono::freertos_clock::now();
        auto t = now.time_since_epoch();
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

                pbuf_chain(p, p_ram);

                ESP_LOGI(TAG, "Retransmitting: p_ram ref count=%u, tot_len=%u",
                    p_ram->ref, p_ram->tot_len);

                ESP_LOGI(TAG, "Retransmitting: counter %u, ref count=%u, p=%p, tot_len=%u",
                    tracked->retransmission_counter,
                    p->ref,
                    p,
                    p->tot_len
                    );

                // "ROM" send seems to work better, though there's a glitch
                // where it fails 2nd send every time.  Feels like it happens
                // when system is already trying to send from that RAM address.
                // Maybe though it's due to our glitchy endpoint (0.0.0.0)
                // DEBT: IIRC ipaddr pointers don't live much past the udp_recv
                // event
                err_t r = pcb.send_experimental(
                    p,
                    tracked->endpoint()
                    );

                ESP_LOGI(TAG, "Retransmitting: phase 1 r=%d, ref count=%u",
                    r, p->ref);

                pbuf_free(p);

                bool processed = tracker.mark_con_sent();

                ESP_LOGI(TAG, "Retransmitting: processed=%u", processed);
            }
        }
    }
}