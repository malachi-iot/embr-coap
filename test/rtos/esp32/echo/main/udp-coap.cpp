#include "esp_log.h"

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include <embr/streambuf.h>

#include <coap/header.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

// TODO: Put specialization of finalize into more readily available area
#include <coap/platform/lwip/encoder.h>

using namespace embr;
using namespace embr::coap;

using embr::lwip::upgrading::ipbuf_streambuf;
using embr::lwip::upgrading::opbuf_streambuf;

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

        // Remember, at this time pbuf code makes no assumptions about
        // how large you want that initial buffer.  Very app dependent.  For CoAP,
        // 32 is more than enough for a simple ACK style response.  Now, with chaining
        // it's even less consequential - however we're embedded, so specifying these
        // things explicitly is welcome rather than wasting space
        StreambufEncoder<opbuf_streambuf> encoder(32);

        bool eof;

        eof = decoder.process_iterate_streambuf().eof;
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf().eof;

        Header header = decoder.header_decoder();
        uint8_t tkl = header.token_length();

        ESP_LOGI(TAG, "state = %s / mid = %d / tkl = %d", 
            get_description(decoder.state()),
            header.message_id(),
            tkl);
        eof = decoder.process_iterate_streambuf().eof;
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf().eof;
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));
        eof = decoder.process_iterate_streambuf().eof;
        ESP_LOGI(TAG, "state = %s", get_description(decoder.state()));

        const uint8_t* token = decoder.token_decoder().data();

        ESP_LOG_BUFFER_HEX(TAG, token, tkl);
        eof = decoder.process_iterate_streambuf().eof;
        ESP_LOGI(TAG, "state = %s, eof=%u",
            get_description(decoder.state()),
            eof);

        // these two send npm coap into a tizzy, even it seems in nonconfirmable mode
        //header.type(Header::TypeEnum::Reset);
        //header.type(Header::TypeEnum::NonConfirmable);
        header.type(Header::Types::Acknowledgement);
        //header.code(Header::Code::Empty);
        header.code(Header::Code::Valid);

        encoder.header(header);
        encoder.token(token, header.token_length());
        encoder.finalize();

        auto& pbuf = encoder.rdbuf()->pbuf();

        ESP_LOGI(TAG, "about to output %d bytes", pbuf.total_length());

        udp_sendto(pcb, pbuf.pbuf(), addr, port);
    }
}

