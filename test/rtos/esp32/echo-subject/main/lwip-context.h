#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/streambuf.h>
#include <coap/decoder/streambuf.h>
#include <coap/encoder/streambuf.h>

#define COAP_UDP_PORT 5683

struct LwipContext
{
    typedef struct udp_pcb* pcb_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef embr::lwip::out_pbuf_streambuf<char> out_streambuf_type;
    typedef embr::lwip::in_pbuf_streambuf<char> in_streambuf_type;
    typedef out_streambuf_type::size_type size_type;

    typedef embr::coap::experimental::StreambufEncoder<out_streambuf_type> encoder_type;
    typedef moducom::coap::experimental::StreambufDecoder<in_streambuf_type> decoder_type;

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
        while(decoder.state() != moducom::coap::Decoder::Done);
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

// in C# we'd do an IEncoderFactory, in C++ for tiny code footprint
// we'll instead do a special override of make_encoder.  Important since
// the particulars of how an encoder+streambuf is initialized changes
// for the underlying streambuf+application in question

inline LwipContext::encoder_type make_encoder(const LwipContext&)
{
    // stock-standard size is 256, which is generally too large
    // for many CoAP scenarios but still small enough to throw around
    // in a memory constrained system.  One can (and should) specialize
    // their context for more specificity
    return LwipContext::encoder_type(256);
}
