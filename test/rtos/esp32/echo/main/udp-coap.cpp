#include "esp_log.h"

#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include <coap/header.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

#define COAP_UDP_PORT 5683

using namespace embr;
using namespace embr::mem;
using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace embr::coap::experimental;

typedef embr::lwip::PbufNetbuf netbuf_type;
typedef netbuf_type::size_type size_type;

typedef out_netbuf_streambuf<char, netbuf_type> out_pbuf_streambuf;
typedef in_netbuf_streambuf<char, netbuf_type> in_pbuf_streambuf;

typedef estd::internal::basic_ostream<out_pbuf_streambuf> pbuf_ostream;
typedef estd::internal::basic_istream<in_pbuf_streambuf> pbuf_istream;

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    if (p != NULL)
    {
        ESP_LOGI(TAG, "entry: p->len=%d", p->len);
        // Not required for latest streambuf decoder code, and will be phased out
        int dummy_length = 0;

        // will auto-free p since it's not bumping reference
        //pbuf_istream in(p, false);
        // WARN: Be careful!!! this compiles, but seems to map to
        //       "const PbufNetbuf& copy_from, bool reset, bool bump_reference = true"
        //       signature
        //StreambufDecoder<in_pbuf_streambuf> decoder(0, p, false);

        StreambufDecoder<in_pbuf_streambuf> decoder(p, false);
        // Remember, at this time netbuf-pbuf code makes no assumptions about
        // how large you want that initial buffer.  Very app dependent.  For CoAP,
        // 32 is more than enough for a simple ACK style response.  Now, with chaining
        // it's even less consequential - however we're embedded, so specifying these
        // things explicitly is welcome rather than wasting space
        StreambufEncoder<out_pbuf_streambuf> encoder(32);

        bool eof;

        eof = decoder.process_iterate_streambuf();
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf();

        Header header = decoder.header_decoder();
        uint8_t tkl = header.token_length();

        ESP_LOGI(TAG, "state = %s / mid = %d / tkl = %d", 
            get_description(decoder.state()),
            header.message_id(),
            tkl);
        eof = decoder.process_iterate_streambuf();
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf();
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf();
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));

        const uint8_t* token = decoder.token_decoder().data();

        ESP_LOG_BUFFER_HEX(TAG, token, tkl);
        eof = decoder.process_iterate_streambuf();
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));

        // these two send npm coap into a tizzy, even it seems in nonconfirmable mode
        //header.type(Header::TypeEnum::Reset);
        //header.type(Header::TypeEnum::NonConfirmable);
        header.type(Header::TypeEnum::Acknowledgement);
        //header.code(Header::Code::Empty);
        header.code(Header::Code::Valid);

        encoder.header(header);
        encoder.token(token, header.token_length());
        encoder.finalize_experimental();

        netbuf_type& netbuf = encoder.rdbuf()->netbuf();

        size_type absolute_pos = encoder.rdbuf()->absolute_pos();

        netbuf.shrink(absolute_pos);

        ESP_LOGI(TAG, "about to output %d bytes (absolute_pos=%d)", 
            netbuf.total_size(),
            absolute_pos);

        // NOTE: I think this works, I can't remember
        // waiting to look up CoAP 'ping' type operation - I do recall
        // needing to change header 'ack' around
        udp_sendto(pcb, netbuf.pbuf(), addr, port);
    }
}

void udp_coap_init(void)
{
    struct udp_pcb * pcb;

    /* get new pcb */
    pcb = udp_new();
    if (pcb == NULL) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_new failed!\n"));
        return;
    }

    /* bind to any IP address */
    if (udp_bind(pcb, IP_ADDR_ANY, COAP_UDP_PORT) != ERR_OK) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_bind failed!\n"));
        return;
    }

    /* set udp_echo_recv() as callback function
       for received packets */
    udp_recv(pcb, udp_coap_recv, NULL);
}