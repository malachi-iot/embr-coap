#include <exp/datapump.hpp>
#include <platform/lwip/lwip-datapump.h>
#include <lwip/api.hpp>

using namespace moducom::coap;

#define COAP_UDP_PORT 5683

typedef lwip_datapump_t::netbuf_t netbuf_t;
typedef lwip_datapump_t::addr_t addr_t;

void nonblocking_datapump_loop(lwip::Netconn netconn, lwip_datapump_t& datapump)
{
    addr_t addr;
    const netbuf_t* netbuf_out = datapump.transport_out(&addr);
    //lwip::Netbuf netbuf_native(netbuf_out->native());
    netbuf* native_netbuf;

    err_t err;

    if(netbuf_out != NULLPTR)
    {
        err = netconn.sendTo(NULLPTR, &addr.addr, addr.port);
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

    datapump.transport_in(netbuf_in, addr);
}

lwip_datapump_t datapump;

extern "C" void coap_daemon(void *)
{
    lwip::Netconn conn(NETCONN_UDP);

    conn.bind(nullptr, COAP_UDP_PORT);

    // TODO: improve this by using callbacks/nonblocking functions
    // utilizing technique as described here https://stackoverflow.com/questions/10848851/lwip-rtos-how-to-avoid-netconn-block-the-thread-forever
    // but we are not using accept, so hopefully same approach applies to a direct
    // recv call
    conn.set_recvtimeout(10);

    for(;;)
    {
        nonblocking_datapump_loop(conn, datapump);
    }
}