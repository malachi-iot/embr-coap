#include "lwip-datapump.h"

#ifdef FEATURE_MC_MEM_LWIP

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

        err = netconn.sendTo(netbuf_out->native(), &addr.addr, addr.port);

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

    LwipNetbuf netbuf_in(native_netbuf, true);

    new (&addr) addr_t(native_netbuf);

    // TODO: Make a specialized "in netbuf" whose length_processed always matches
    // the provided underlying netbuf length.
    // FIX: above will solve also the issue of 'next()' not populating length_processed
    // as needed
    netbuf_in.advance(netbuf_in.length_processed());

    datapump.enqueue_out(netbuf_in, addr);
}

}}

#endif