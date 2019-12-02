#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/pbuf.h>

struct LwipContext
{
    typedef struct udp_pcb* pcb_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef embr::lwip::out_pbuf_streambuf<char> out_streambuf_type;
    typedef embr::lwip::in_pbuf_streambuf<char> in_streambuf_type;
    typedef out_streambuf_type::size_type size_type;

    typedef StreambufEncoder<out_streambuf_type> encoder_type;
    typedef StreambufDecoder<in_streambuf_type> decoder_type;

    pcb_pointer pcb;
    uint16_t port;

    LwipContext(pcb_pointer pcb,
        uint16_t port) : 
        pcb(pcb),
        port(port) {}

    // most times in a udp_recv handler we're expected to issue a free on
    // pbuf also.  bumpref = false stops us from auto-bumping ref with our
    // pbuf-netbuf, so that when it leaves scope it issues a pbuf_free
    template <class TSubject, class TContext>
    static void do_notify(
        TSubject& subject, TContext& context,
        pbuf_pointer incoming, bool bumpref = false)
    {
        decoder_type decoder(incoming, bumpref);

        do
        {
            decode_and_notify(decoder, subject, context);
        }
        while(decoder.state() != Decoder::Done);
    }

    void sendto(encoder_type& encoder, 
        const ip_addr_t* addr,
        uint16_t port)
    {
        udp_sendto(pcb, 
            encoder.rdbuf()->netbuf().pbuf(), 
            addr, 
            port);
    }
};

struct AppContext : 
    moducom::coap::IncomingContext<const ip_addr_t*>,
    LwipContext
{
    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb, port)
    {
        this->addr = addr;
    }

    // number of seed bytes is sorta app-specific, so do it here
    // (ping needs only 8 bytes ever)
    encoder_type make_encoder()
    {
        return encoder_type(8);
    }

    void reply(encoder_type& encoder)
    {
        sendto(encoder, this->address(), port);
    }


    template <class TSubject>
    void do_notify(pbuf_pointer p, TSubject& subject)
    {
        LwipContext::do_notify(subject, *this, p);
    }
};