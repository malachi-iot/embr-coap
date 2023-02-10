#include <string>
#include <cstring>

#include <esp_crc.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_random.h>
#include <esp_wifi.h>

#include <estd/port/freertos/timer.h>
//#include <estd/platform/freertos/timer.h>

using namespace estd::chrono_literals;

static void outbound_ping_fn(TimerHandle_t t);

static estd::freertos::timer<true> outbound_ping
    ("outbound esp-now ping", 5s, true, nullptr,
    outbound_ping_fn);

static const char* TAG = "echo: esp-now";

// Guidance from
// https://github.com/espressif/esp-idf/tree/v5.0/examples/wifi/espnow



#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

enum
{
    EXAMPLE_ESPNOW_DATA_BROADCAST,
    EXAMPLE_ESPNOW_DATA_UNICAST,
    EXAMPLE_ESPNOW_DATA_MAX,
};

/* User defined field of ESPNOW data in this example. */
typedef struct
{
    uint8_t type;                         //Broadcast or unicast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint16_t seq_num;                     //Sequence number of ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint8_t payload[0];                   //Real payload of ESPNOW data.
} __attribute__((packed)) example_espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct
{
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} example_espnow_send_param_t;


static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };


/* Prepare ESPNOW data to be sent. */
static void example_espnow_data_prepare(example_espnow_send_param_t *send_param)
{
    example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(example_espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    /* Fill all remaining bytes after the data with random values */
    esp_fill_random(buf->payload, send_param->len - sizeof(example_espnow_data_t));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGD(TAG, "send_cb: %u", status);
}

static void recv_cb(
    // Differs from 'master' example, this line doesn't compile
    //const esp_now_peer_info_t *recv_info, const uint8_t *data, int len)
    // v5.0 flavor:
    const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
    ESP_LOGI(TAG, "recv_cb: %02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
        mac_addr[4], mac_addr[5]);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_DEBUG);
}


/* Must run before wifi start is called
// TODO: Go fixup esp_helper's event handler to be static
static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "event_handler: reached");
    
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_CONNECTED:
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Scan done, now we can align peer channel");
                ESP_ERROR_CHECK(esp_wifi_set_channel(
                    CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

                break;

            default: break;
        }
    }
} */

void init_esp_now()
{
    ESP_LOGI(TAG, "init_esp_now: entry");

    // This particular tidbit is from master branch, seems to crash things
    // with esp-idf v5.0
    //ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    uint8_t primary_channel;
    wifi_second_chan_t second;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&primary_channel, &second));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol((wifi_interface_t)ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
#endif

    // For v5.0 we need to do it while NOT scanning
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    esp_now_peer_info_t broadcast_peer = {};

    //broadcast_peer.channel = CONFIG_ESPNOW_CHANNEL;
    broadcast_peer.channel = primary_channel;
    broadcast_peer.ifidx = (wifi_interface_t)ESPNOW_WIFI_IF;
    broadcast_peer.encrypt = false;

    std::copy_n(s_broadcast_mac, 6, broadcast_peer.peer_addr);

    ESP_ERROR_CHECK(esp_now_add_peer(&broadcast_peer));

    /* Initialize sending parameters. */
    example_espnow_send_param_t* send_param = new example_espnow_send_param_t;
    
    if (send_param == NULL)
    {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        //vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        //return ESP_FAIL;
        return;
    }
    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param->len = CONFIG_ESPNOW_SEND_LEN;
    send_param->buffer = (uint8_t*)malloc(CONFIG_ESPNOW_SEND_LEN);
    
    if (send_param->buffer == NULL)
    {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        delete send_param;
        //vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        //return ESP_FAIL;
        return;
    }
    
    memcpy(send_param->dest_mac, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    example_espnow_data_prepare(send_param);

    outbound_ping.start(200ms);

    ESP_LOGI(TAG, "init_esp_now: exit");
}


static void outbound_ping_fn(TimerHandle_t t)
{
    static const char s[] = "Hello World!";

    // No matter what we do, we get
    // "Peer channel is not equal to the home channel, send fail!"
    // even after trying 3 or 4 different placements of
    // ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    // (most of which just crash)
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_now_send(
        //nullptr,
        &s_broadcast_mac[0],
        (const uint8_t*)s, sizeof(s) - 1));
    ESP_LOGV(TAG, "outbound_ping_fn: send queued");
}
