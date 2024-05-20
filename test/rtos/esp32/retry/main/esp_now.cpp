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

#define CONFIG_ESPNOW_CHANNEL           1
#define CONFIG_ESPNOW_WIFI_MODE_STATION 1

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

static const char* TAG = "app::esp_now";

namespace app::esp_now {

// https://docs.espressif.com/projects/esp-idf/en/v5.1.4/esp32/api-reference/network/esp_now.html#_CPPv417esp_now_peer_info
class peer_info
{
    esp_now_peer_info_t info_ {};

public:
    peer_info() = default;

    // DEBT: Use c++ specific array ref syntax here to really, really enforce len
    // may have double bonus of being constexpr'able
    peer_info(const uint8_t mac[ESP_NOW_ETH_ALEN], wifi_interface_t ifidx, uint8_t channel = 0)
    {
        info_.ifidx = ifidx;
        info_.channel = channel;
        std::copy_n(mac, ESP_NOW_ETH_ALEN, info_.peer_addr);
    }

    // EXPERIMENTAL - I don't think I like it.  esp_now_add_peer wrapped as "add" would
    // feel like a huge side effect
    esp_err_t add()
    {
        return esp_now_add_peer(&info_);
    }

    // EXPERIMENTAL
    esp_err_t get(const uint8_t mac[ESP_NOW_ETH_ALEN])
    {
        return esp_now_get_peer(mac, &info_);
    }

    void lmk(uint8_t key[ESP_NOW_KEY_LEN])
    {
        info_.encrypt = true;
        std::copy_n(key, ESP_NOW_KEY_LEN, info_.lmk);
    }
};

#define ESP_NOW_BROADCAST_MAC   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

static constexpr uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] { ESP_NOW_BROADCAST_MAC };

// DEBT: Make a constexpr constructo for 'Header' and use that here
const static uint8_t buffer_coap_ping[] =
{
    0x40, coap::Header::Code::Ping, 0x01, 0x23, // mid = 123, tkl = 0, GET, CON
};

enum Roles
{
    ROLE_TARGET,
    ROLE_INITIATOR
};

static Roles role;

tracker_type tracker;

static void init_protocol();

void init(void)
{
    // esp_helper does a lot of this already, thus the DEBT that we cannot
    // activate UDP/IP and ESP-NOW at once
    
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

const char* to_string(coap::Header::Code code)
{
    using Code = coap::Header::Code;

    switch(code)
    {
        case Code::BadRequest:  return "Bad Request";
        case Code::Created:     return "Created";
        case Code::Empty:       return "Empty";
        case Code::Get:         return "Get";
        case Code::Ping:        return "Ping";
        case Code::Pong:        return "Pong";
        case Code::Valid:       return "Valid";

        default:    return "N/A";
    }
}

static void recv_cb(const esp_now_recv_info_t *recv_info,
    const uint8_t *data, int len)
{
    // See above DEBT
    //send_ack(estd::span<char>{(char*)data, (unsigned)len});
    coap::Header header;

    std::copy_n(data, 4, header.bytes);

    ESP_LOGI(TAG, "recv_cb: len=%d, mid=0x%" PRIx16 ", code=%s",
        len,
        header.message_id(),
        to_string(header.code())
        );

    switch(header.code())
    {
        case coap::Header::Code::Ping:
        {
            if (esp_now_is_peer_exist(recv_info->src_addr) == false)
            {
                peer_info peer(recv_info->src_addr, WIFI_IF_STA, 0);

                ESP_ERROR_CHECK(peer.add());
            }

            header.code(coap::Header::Code::Pong);
            ESP_ERROR_CHECK(esp_now_send(recv_info->src_addr, header.bytes, 4));
            break;
        }
    }
}

inline const char* to_string(esp_err_t code)
{
    return esp_err_to_name(code);
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

    const esp_now_peer_info_t broadcast_peer
    {
        .peer_addr { ESP_NOW_BROADCAST_MAC },
        .lmk {},
        .channel = 0,
        .ifidx = WIFI_IF_STA,
        .encrypt = 0,
        .priv = nullptr
    };

    ESP_ERROR_CHECK(esp_now_add_peer(&broadcast_peer));

    esp_err_t r = esp_now_send(broadcast_mac, buffer_coap_ping, sizeof(buffer_coap_ping));

    ESP_LOGI(TAG, "init_protocol: ping send result: %s", to_string(r));
}


void loop(duration t)
{
    //send_ack();
}

}