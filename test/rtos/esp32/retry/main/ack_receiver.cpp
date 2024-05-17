#include "esp_log.h"

#include <esp-helper.h>

#include <estd/chrono.h>

#include <embr/platform/lwip/pbuf.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/udp.h>

// DEBT: At the moment, this is where StreambufProvider specialization lives
#include <coap/platform/lwip/factory.h>
#include <coap/platform/ip.h>

#include <coap/header.h>

#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

// DEBT: Put specialization of finalize into more readily available area
#include <coap/platform/lwip/encoder.h>

#include "app.h"

using namespace embr;
using namespace embr::coap::experimental;

using embr::lwip::ipbuf_streambuf;
using embr::lwip::opbuf_streambuf;

#if FEATURE_RETRY_TRACKER_V2
tracker_type tracker;
#endif

template <class Streambuf>
bool until(coap::StreambufDecoder<Streambuf>& decoder, coap::Decoder::State state)
{
    coap::iterated::decode_result r;

    do
    {
        r = decoder.process_iterate_streambuf();
    }
    while(decoder.state() != state && r.eof == false);

    return r.eof == false;
}

static void send_ack(embr::lwip::udp::Pcb pcb, embr::lwip::Pbuf& pbuf,
    const endpoint_type& endpoint)
{
    static const char* TAG = "send_ack";

    coap::StreambufDecoder<ipbuf_streambuf> decoder(pbuf);

    until(decoder, coap::Decoder::HeaderDone);
    coap::Header header = decoder.header_decoder();
    until(decoder, coap::Decoder::TokenDone);
    const uint8_t* token = decoder.token_decoder().data();

    coap::StreambufEncoder<opbuf_streambuf> encoder(32);

    header.type(coap::Header::Types::Acknowledgement);
    header.code(coap::Header::Code::Valid);

    encoder.header(header);
    encoder.token(token, header.token_length());
    encoder.finalize();

    auto& encoder_pbuf = encoder.rdbuf()->pbuf();

    ESP_LOGI(TAG, "about to output %d bytes", encoder_pbuf.total_length());

    pcb.send_experimental(encoder_pbuf.pbuf(), endpoint);
}

static void send_echo_with_con(embr::lwip::udp::Pcb pcb, embr::lwip::Pbuf& pbuf,
    const endpoint_type& endpoint)
{
    static const char* TAG = "send_con";
    
    coap::StreambufDecoder<ipbuf_streambuf> decoder(pbuf);

    until(decoder, coap::Decoder::HeaderDone);
    coap::Header header = decoder.header_decoder();
    until(decoder, coap::Decoder::TokenDone);
    const uint8_t* token = decoder.token_decoder().data();

    if(header.type() == coap::Header::Reset)  return;

    coap::StreambufEncoder<opbuf_streambuf> encoder(32);

    header.type(coap::Header::Types::Confirmable);
    header.code(coap::Header::Code::Valid);
    // DEBT: Pretty lousy way to generate a different message ID
    header.message_id(header.message_id() + 1);

    encoder.header(header);
    encoder.token(token, header.token_length());
    encoder.finalize();

    auto& encoder_pbuf = encoder.rdbuf()->pbuf();

    ESP_LOGI(TAG, "about to output %d bytes", encoder_pbuf.total_length());

    // "pbufs passed to IP must have a ref-count of 1 as their payload pointer
    // gets altered as the packet is passed down the stack"
    //encoder_pbuf.ref();

    pcb.send_experimental(encoder_pbuf, endpoint);
    auto now = estd::chrono::freertos_clock::now();
    auto t = now.time_since_epoch();
    
    // pbuf does not get deallocated automatically on send, according to these guys
    // https://stackoverflow.com/questions/71763904/lwip-when-to-deallocate-the-pbuf-after-calling-udp-sendto
    // https://lwip.fandom.com/wiki/Raw/UDP

    const struct pbuf* p = encoder_pbuf;

    ESP_LOGI(TAG, "exit: phase 1 ref count=%u", p->ref);

    tracker.track(t, endpoint, std::move(encoder_pbuf));
    //encoder_pbuf.ref();

    ESP_LOGI(TAG, "exit: phase 2 ref count=%u", p->ref);
}

void udp_coap_recv(void *arg, 
    struct udp_pcb *_pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    static const char* TAG = "udp_coap_recv";

    ESP_LOGI(TAG, "entry");

    const endpoint_type endpoint(addr, port);

    embr::lwip::udp::Pcb pcb(_pcb);
    embr::lwip::Pbuf pbuf(p);
    
    embr::coap::Header header = embr::coap::experimental::get_header(pbuf);

    ESP_LOGD(TAG, "mid=%x, type=%u", header.message_id(), header.type());

    if(header.type() == coap::Header::Reset)    return;

    // First, send ACK
    send_ack(pcb, pbuf, endpoint);

    // Next, send actual CON message in other direction
    send_echo_with_con(pcb, pbuf, endpoint);

#if FEATURE_RETRY_MANAGER
    auto manager = (manager_type*) arg;
#endif
#if FEATURE_RETRY_TRACKER_V2
    if(header.type() == coap::Header::Acknowledgement)
    {
        bool r = tracker.ack_encountered(endpoint, header.message_id());

        ESP_LOGD(TAG, "ack_encountered found tracked=%u", r);
    }
#endif
}