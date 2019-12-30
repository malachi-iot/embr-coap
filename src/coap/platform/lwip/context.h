#pragma once

// On to use experimental LWIP transport abstraction code
#define FEATURE_MCCOAP_LWIP_TRANSPORT

#include <coap/context.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/transport.h>
#include <coap/decoder/streambuf.h>
#include <coap/encoder/streambuf.h>

// This is all UDP oriented.  No TCP or SSL support

#define COAP_UDP_PORT 5683

namespace moducom { namespace coap {

// TODO: Either include addr in here and somehow NOT in app context,
// or go other direction and inherit from moducom::coap::IncomingContext
// - If we do the former, except for encoder/decoder types this could be
//   completely decoupled from coap
// - If we do the latter, then this could more comfortably contain the
//   reply data which AppContext needs
struct LwipContext
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
    : embr::lwip::experimental::TransportUdp
#endif
{
    typedef struct udp_pcb* pcb_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef embr::lwip::opbuf_streambuf out_streambuf_type;
    typedef embr::lwip::ipbuf_streambuf in_streambuf_type;
    typedef out_streambuf_type::size_type size_type;

    typedef moducom::coap::StreambufEncoder<out_streambuf_type> encoder_type;
    typedef moducom::coap::StreambufDecoder<in_streambuf_type> decoder_type;

#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
    typedef embr::lwip::experimental::TransportUdp::endpoint_type PortAndAddress;

    LwipContext(pcb_pointer pcb) : 
        embr::lwip::experimental::TransportUdp(pcb)
        {}
#else
    struct PortAndAddress
    {
        const ip_addr_t* addr;
        uint16_t port;
    };

    pcb_pointer pcb;

    LwipContext(pcb_pointer pcb) : 
        pcb(pcb)
        {}
#endif

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

#ifndef FEATURE_MCCOAP_LWIP_TRANSPORT
    void sendto(encoder_type& encoder, 
        const ip_addr_t* addr,
        uint16_t port)
    {
        udp_sendto(pcb, 
            encoder.rdbuf()->netbuf().pbuf(), 
            addr, 
            port);
    }
#endif
};


// We're tracking from-addr and from-port since CoAP likes to respond
// to that 
struct LwipIncomingContext :
    moducom::coap::IncomingContext<LwipContext::PortAndAddress>,
    LwipContext
{
    typedef LwipContext::PortAndAddress addr_type;

    LwipIncomingContext(pcb_pointer pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb)
    {
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
        this->addr.address = addr;
#else
        this->addr.addr = addr;
#endif
        this->addr.port = port;
    }


    LwipIncomingContext(pcb_pointer pcb, const addr_type& addr) :
        LwipContext(pcb)
    {
        this->addr = addr;
    }

    void reply(encoder_type& encoder)
    {
        encoder.finalize();

        const addr_type& addr = this->address();
        
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
        send(*encoder.rdbuf(), addr);
#else
        sendto(encoder, addr.addr, addr.port);
#endif
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

}}