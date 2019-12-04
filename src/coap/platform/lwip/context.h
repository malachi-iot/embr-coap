#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/streambuf.h>
#include <coap/decoder/streambuf.h>
#include <coap/encoder/streambuf.h>

// This is all UDP oriented.  No TCP or SSL support

#define COAP_UDP_PORT 5683

// TODO: Do namespacing!
//namespace moducom { namespace coap {

// TODO: Either include addr in here and somehow NOT in app context,
// or go other direction and inherit from moducom::coap::IncomingContext
// - If we do the former, except for encoder/decoder types this could be
//   completely decoupled from coap
// - If we do the latter, then this could more comfortably contain the
//   reply data which AppContext needs
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


// We're tracking from-addr and from-port since CoAP likes to respond
// to that 
struct LwipIncomingContext :
    moducom::coap::IncomingContext<const ip_addr_t*>,
    LwipContext
{
    LwipIncomingContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb, port)
    {
        this->addr = addr;
    }

    void reply(encoder_type& encoder)
    {
        encoder.finalize();
        
        sendto(encoder, this->address(), port);
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


template <bool inline_token, class TStreambuf>
inline void build_reply(
    const moducom::coap::TokenAndHeaderContext<inline_token>& context, 
    embr::coap::experimental::StreambufEncoder<TStreambuf>& encoder, uint8_t code)
{
    typedef moducom::coap::Header Header;

    Header header = context.header();
    auto token = context.token();

    header.code(code);
    header.type(Header::TypeEnum::Acknowledgement);

    encoder.header(header);
    encoder.token(token);
}

// Expects TContext to be/conform to:
// moducom::coap::IncomingContext
// TODO: better suited to cpp/hpp - be nice to non-inline it
// NOTE: leans heavily on RVO, at least as much as 'make_encoder' itself does
template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(const TContext& context, uint8_t code)
{
    typename TContext::encoder_type encoder = make_encoder(context);

    build_encoder_reply(context, encoder, code);

    return encoder;
}

//}}