#include "lwip-datapump.h"

#ifdef FEATURE_MC_MEM_LWIP

using namespace moducom::mem;

namespace moducom { namespace coap {

typedef lwip_datapump_t::netbuf_t netbuf_t;
typedef lwip_datapump_t::addr_t addr_t;

void nonblocking_datapump_loop(lwip::Netconn netconn, lwip_datapump_t& datapump)
{
    addr_t addr;
    const netbuf_t* netbuf_out = datapump.transport_front(&addr);
    //lwip::Netbuf netbuf_native(netbuf_out->native());
    netbuf* native_netbuf;

    err_t err;

    if(netbuf_out != NULLPTR)
    {
        printf("nonblocking_datapump_loop: send to port: %d\n", addr.port);

        native_netbuf = netbuf_out->native();

        // Two things to note:
        // a) pbuf_realloc(native_netbuf->p) is *slightly* out of spec,
        //    but it's the only way I know to shrink-to-fit the netbuf to the right size for sending
        // b) netbuf_out.length_processed is *not* total length, only the encoded length
        //    of the last chunk.  This is a FIX because it WILL break soon 
        pbuf_realloc(native_netbuf->p, netbuf_out->length_processed());

        // FIX: netconn+netbuf setup such that proper size is tricky to send.
        // netbuf requires pre-allocation, nearly demanding you don't fill the
        // entire buf, but then when it comes time to send there's no specifier
        // for sending less than the full buffer.  Furthermore, this creates
        // a firm problem since CoAP uses UDP packet size to determine payload
        // size
        err = netconn.sendTo(native_netbuf, &addr.addr, addr.port);

        printf("err=%d\n", err);

        datapump.transport_pop();
    }

    err = netconn.recv(&native_netbuf);

    if(err == ERR_TIMEOUT) return;

    // TODO: allocate a mc-mem netbuf in which to wrap the native netbuf
    // OR make TNetBuf a value instead of pointer within datapump itself
    // that might be more viable and in the edge case where TNetBuf really
    // is too unweildy (wants to be a pointer) could make a wrapper for it
    // in that instance

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    LwipNetbuf netbuf_in(native_netbuf, true);
#else
    delete netbuf_out;

    LwipNetbuf* allocated = new LwipNetbuf(native_netbuf, true);
    LwipNetbuf& netbuf_in = *allocated;
#endif

    // initialize address with the address from incoming netbuf
    new (&addr) addr_t(native_netbuf);

    // TODO: Make a specialized "in netbuf" whose length_processed always matches
    // the provided underlying netbuf length.
    // FIX: above will solve also the issue of 'next()' not populating length_processed
    // as needed
    // NOTE: This is affected possibly by lwip netbuf semi-inflexibility on precise
    // buf length (plan is to peer into underlying PBUF and reduce tot_len)
    netbuf_in.advance(netbuf_in.length_processed());

    datapump.enqueue_out(netbuf_in, addr);
}

}}

#endif