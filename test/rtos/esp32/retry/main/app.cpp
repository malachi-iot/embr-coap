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
                struct pbuf* p = tracked->buffer();

                ESP_LOGI(TAG, "Retransmitting: counter %u, ref count=%u, p=%p, len=%u",
                    tracked->retransmission_counter,
                    p->ref,
                    p,
                    p->len
                    );

                err_t r = pcb.send_experimental(
                    tracked->buffer(),
                    tracked->endpoint()
                    );

                ESP_LOGI(TAG, "Retransmitting: phase 1 %d, ref count=%u",
                    r, p->ref);

                tracker.mark_con_sent();
            }
        }
    }
}