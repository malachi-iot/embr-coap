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
#include "senders.hpp"

using namespace embr;
using namespace embr::coap::experimental;

using embr::lwip::ipbuf_streambuf;
using embr::lwip::opbuf_streambuf;

#if FEATURE_RETRY_TRACKER_V2
tracker_type tracker;
#endif

static void send_ack(embr::lwip::udp::Pcb pcb, embr::lwip::Pbuf& pbuf,
    const endpoint_type& endpoint)
{
    static const char* TAG = "send_ack";

    coap::StreambufDecoder<ipbuf_streambuf> decoder(pbuf);
    coap::StreambufEncoder<opbuf_streambuf> encoder(32);

    build_ack(decoder, encoder);

    auto& encoder_pbuf = encoder.rdbuf()->pbuf();

    ESP_LOGI(TAG, "about to output %d bytes", encoder_pbuf.total_length());

    pcb.send_experimental(encoder_pbuf.pbuf(), endpoint);
}

static void send_echo_with_con(embr::lwip::udp::Pcb pcb, embr::lwip::Pbuf& pbuf,
    const endpoint_type& endpoint)
{
    static const char* TAG = "send_con";
    
    coap::StreambufDecoder<ipbuf_streambuf> decoder(pbuf);
    // One might think PBUF_RAW or PBUF_RAW_TX could be used here, since
    // we're chaining here from PBUF_ROM.  One would be mistaken, it turns out
    coap::StreambufEncoder<opbuf_streambuf> encoder(32);

    if(build_con(decoder, encoder) == false)    return;

    auto& encoder_pbuf = encoder.rdbuf()->pbuf();
    err_t r;

    struct pbuf* p = encoder_pbuf;
    struct pbuf* p_rom = pbuf_alloc(
        PBUF_TRANSPORT,
        //encoder_pbuf.total_length(),
        0,
        PBUF_ROM);

    pbuf_chain(p_rom, p);

    //struct pbuf workaround = *p;

    ESP_LOGI(TAG, "about to output %d bytes, ref count=%u",
        encoder_pbuf.total_length(),
        p->ref
        );

    ESP_LOGI(TAG, "chained: tot_len=%u", p_rom->tot_len);

    // "pbufs passed to IP must have a ref-count of 1 as their payload pointer
    // gets altered as the packet is passed down the stack"
    //encoder_pbuf.ref();

    // Seeing if we can use ROM approach to avoid side-effect of udp_sendto a little
    // Beware not all platforms support ROM approach https://doc.ecoscentric.com/ref/lwip-basics.html

    r = pcb.send_experimental(p_rom, endpoint);
    pbuf_free(p_rom);
    auto now = estd::chrono::freertos_clock::now();
    auto t = now.time_since_epoch();

    // https://stackoverflow.com/questions/68786784/is-it-possible-to-use-udp-sendto-to-resend-an-udp-packet
    // https://lwip-users.nongnu.narkive.com/bexfjxTs/udp-send-space-for-ip-header-is-not-being-allocated

    //*p = workaround;
    //r = pbuf_header(p, -UDP_HLEN);
    
    // pbuf does not get deallocated automatically on send, according to these guys
    // https://stackoverflow.com/questions/71763904/lwip-when-to-deallocate-the-pbuf-after-calling-udp-sendto
    // https://lwip.fandom.com/wiki/Raw/UDP

    ESP_LOGI(TAG, "exit: phase 2 r=%d, ref count=%u, p=%p, tot_len=%u",
        r, p->ref, p, p->tot_len);

    //encoder_pbuf.ref();
    tracker.track(t, endpoint, std::move(encoder_pbuf));
    //pbuf_ref(p);

    ESP_LOGI(TAG, "exit: phase 3 ref count=%u, tot_len=%u", p->ref, p->tot_len);
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