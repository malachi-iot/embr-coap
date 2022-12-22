#pragma once

// Default on to use experimental LwIP transport abstraction code
#ifndef FEATURE_MCCOAP_LWIP_TRANSPORT
#define FEATURE_MCCOAP_LWIP_TRANSPORT 1
#endif

#include <coap/context.h>
#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/transport.h>
#include <coap/decoder/streambuf.h>
#include <coap/encoder/streambuf.h>

#include "../ip.h"

#include <exp/retry/factory.h>

// This is all UDP oriented.  No TCP or SSL support

namespace embr { namespace coap {

// most times in a udp_recv handler we're expected to issue a free on
// pbuf also.  bumpref = false stops us from auto-bumping ref with our
// pbuf-netbuf, so that when it leaves scope it issues a pbuf_free
template <class TSubject, class TContext>
inline static iterated::decode_result decode_and_notify(
    struct pbuf* incoming,
    TSubject& subject, TContext& context,
    bool bumpref = false)
{
    typedef StreambufDecoder<embr::lwip::ipbuf_streambuf> decoder_type;
    decoder_type decoder(incoming, bumpref);

    return decode_and_notify(decoder, subject, context);
}


#ifdef __cpp_rvalue_references
template <class TSubject, class TContext>
static iterated::decode_result decode_and_notify(
    struct pbuf* incoming,
    TSubject&& subject, TContext& context,
    bool bumpref = false)
{
    typedef StreambufDecoder<embr::lwip::ipbuf_streambuf> decoder_type;
    decoder_type decoder(incoming, bumpref);

    return decode_and_notify(decoder, std::move(subject), context);
}
#endif

namespace experimental {

template <>
struct StreambufProvider<struct pbuf*>
{
    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;
};


template <unsigned N>
struct LwipPbufFactory
{
    static inline embr::lwip::Pbuf create()
    {
        return embr::lwip::Pbuf(N);
    }
};

}


// TODO: Either include addr in here and somehow NOT in app context,
// or go other direction and inherit from embr::coap::IncomingContext
// - If we do the former, except for encoder/decoder types this could be
//   completely decoupled from coap
// - If we do the latter, then this could more comfortably contain the
//   reply data which AppContext needs
struct LwipContext
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
    : embr::lwip::experimental::TransportUdp<>
#endif
{
    typedef embr::lwip::experimental::TransportUdp<> base_type;
    typedef struct udp_pcb* pcb_pointer;
    typedef struct pbuf* pbuf_pointer;
    typedef typename base_type::ostreambuf_type out_streambuf_type;
    typedef typename base_type::istreambuf_type in_streambuf_type;
    typedef out_streambuf_type::size_type size_type;

    typedef embr::coap::StreambufEncoder<out_streambuf_type> encoder_type;
    typedef embr::coap::StreambufDecoder<in_streambuf_type> decoder_type;

    typedef embr::lwip::internal::Endpoint<> endpoint_type;

#ifdef __cpp_alias_templates
    template <unsigned N>
    using lwip_encoder_factory = embr::coap::experimental::EncoderFactory<
        pbuf_pointer, embr::coap::experimental::LwipPbufFactory<N> >;
#endif
    
    // stock-standard size is 256, which is generally too large
    // for many CoAP scenarios but still small enough to throw around
    // in a memory constrained system.  One can (and should) specialize
    // their context for more specificity
    
    // Default factory, but can be overridden later down hierarchy
    typedef embr::coap::experimental::EncoderFactory<
        pbuf_pointer,
        embr::coap::experimental::LwipPbufFactory<64> > encoder_factory;

#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
    LwipContext(pcb_pointer pcb) : 
        embr::lwip::experimental::TransportUdp<>(pcb)
        {}
#else
    pcb_pointer pcb;

    LwipContext(pcb_pointer pcb) : 
        pcb(pcb)
        {}
#endif

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
    embr::coap::IncomingContext<LwipContext::endpoint_type>,
    LwipContext
{
    typedef LwipContext::endpoint_type endpoint_type;
    typedef embr::coap::IncomingContext<endpoint_type> base_type;

    LwipIncomingContext(pcb_pointer pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
        base_type(endpoint_type(addr, port)),
#endif
        LwipContext(pcb)
    {
#ifndef FEATURE_MCCOAP_LWIP_TRANSPORT
        // NOTE: 09APR22 I believe this is fully obsolete, keeping around
        // just to be conservative
        #warning Obsolete code
        this->addr.addr = addr;
        this->addr.port = port;
#endif
    }


    LwipIncomingContext(pcb_pointer pcb, const endpoint_type& addr) :
        base_type(addr),
        LwipContext(pcb)
    {
    }

    void reply(encoder_type& encoder)
    {
        encoder.finalize();

        const endpoint_type& addr = this->address();
        
#ifdef FEATURE_MCCOAP_LWIP_TRANSPORT
        send(*encoder.rdbuf(), addr);
#else
        sendto(encoder, addr.addr, addr.port);
#endif
    }
};


}}