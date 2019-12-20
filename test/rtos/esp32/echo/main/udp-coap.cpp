#include "esp_log.h"

#include <embr/platform/lwip/iostream.h>
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

typedef embr::lwip::PbufNetbuf netbuf_type;
typedef netbuf_type::size_type size_type;

using embr::lwip::ipbuf_streambuf;
using embr::lwip::opbuf_streambuf;

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    if (p != NULL)
    {
        ESP_LOGI(TAG, "entry: p->len=%d", p->len);

        // _recv plumbing depends on us to frees p,
        // so be sure we do NOT bump up reference, which
        // makes the auto pbuf_free call completely free
        // up p as LwIP wants
        StreambufDecoder<ipbuf_streambuf> decoder(p, false);

        // Remember, at this time netbuf-pbuf code makes no assumptions about
        // how large you want that initial buffer.  Very app dependent.  For CoAP,
        // 32 is more than enough for a simple ACK style response.  Now, with chaining
        // it's even less consequential - however we're embedded, so specifying these
        // things explicitly is welcome rather than wasting space
        StreambufEncoder<opbuf_streambuf> encoder(32);

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
        encoder.finalize();

        netbuf_type& netbuf = encoder.rdbuf()->netbuf();

        ESP_LOGI(TAG, "about to output %d bytes", netbuf.total_size());

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