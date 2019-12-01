#include "esp_log.h"

#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>

#include <embr/observer.h>
// remember to do this and not regular subject.h, otherwise not all the deductions will work
#include <coap/decoder/subject.hpp>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include <coap/header.h>
#include <coap/decoder/streambuf.hpp>
#include <coap/encoder/streambuf.h>

#include "context.h"

// TODO: Time to clean exp events up, functionality is not
// experimental any longer
#include <exp/events.h>


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


struct Observer
{
    void on_notify(header_event, AppContext& context)
    {
    }


    void on_notify(completed_event, AppContext& context)
    {
    }
};

void udp_coap_recv(void *arg, 
    struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    const char* TAG = "udp_coap_recv";

    if (p != NULL)
    {
        StreambufDecoder<in_pbuf_streambuf> decoder(p, false);
        Observer observer;
        // TODO: Need an rvalue (&&) flavor of decode_and_notify so we can use a temp
        // subject
        auto subject = embr::layer1::make_subject(observer);
        // TODO: Spin up proper context so we can get access to a StreambufEncoder somehow
        AppContext context;

        decode_and_notify(decoder, subject);
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