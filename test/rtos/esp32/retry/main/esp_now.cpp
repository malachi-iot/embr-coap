#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_crc.h>

#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

#include "app.h"
#include "senders.hpp"

// https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32/api-reference/network/esp_now.html
// "It is not guaranteed that application layer can receive the data. If necessary, send back ack data when receiving ESP-NOW data."
// Conveniently (for Espressif) they left out HOW exactly to do this ACK technique.  Nothing in the link
// provided or 5 minutes of research indicates there's any ACK or sequence number fields in their action frame
// Max payload size is 250 bytes, and since "The buffer pointed to by data argument does not need to be valid after esp_now_send returns"
// we'll track that on the stack

// Guidance from
// https://github.com/espressif/esp-idf/tree/v5.1.4/examples/wifi/espnow/main

#define CONFIG_ESPNOW_CHANNEL   1

#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

// DEBT: const char support was baked into in_span_streambuf (even though std char_traits doesn't like it)
// however looks like regressions crept in with InStreambuf concept
// https://github.com/malachi-iot/estdlib/issues/40
using istreambuf = estd::detail::streambuf<estd::internal::impl::in_span_streambuf<char> >;
using ostreambuf = estd::detail::streambuf<estd::internal::impl::out_span_streambuf<char> >;

namespace app::esp_now {
tracker_type tracker;

static void init_protocol();

void init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
#endif

    init_protocol();
}


static void send_ack(estd::span<char> in)
{
    static const char* TAG = "send_ack";

    estd::span<char> out;

    coap::StreambufDecoder<istreambuf> decoder(in);
    coap::StreambufEncoder<ostreambuf> encoder(out);

    build_ack(decoder, encoder);
}

static void send_con(estd::span<char> in)
{
    static const char* TAG = "send_ack";

    estd::span<char> out;

    coap::StreambufDecoder<istreambuf> decoder(in);
    coap::StreambufEncoder<ostreambuf> encoder(out);

    if(build_con(decoder, encoder) == false) return;
}

static void recv_cb(const esp_now_recv_info_t *recv_info,
    const uint8_t *data, int len)
{
    // See above DEBT
    send_ack(estd::span<char>{(char*)data, (unsigned)len});
}


static void init_protocol()
{
    ESP_ERROR_CHECK(esp_now_init());
    //ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL));
#endif    
}


void loop(duration t)
{
    //send_ack();
}

}