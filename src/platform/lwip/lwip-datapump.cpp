#include "../../coap/platform.h"

#ifdef FEATURE_MC_MEM_LWIP

#include "lwip-datapump.h"

#ifdef netbuf_copy
#define netbuf_copy_saved
#undef netbuf_copy
#endif

#include "exp/datapump.hpp"

#ifdef netbuf_copy_saved
#define netbuf_copy netbuf_copy_saved
#endif

using namespace moducom::mem;

namespace moducom { namespace coap {

typedef lwip_datapump_t::netbuf_t netbuf_t;
typedef lwip_datapump_t::addr_t addr_t;

// although concerning that we can't pass in a context, should be a non issue
// since we can only have one netconn listening on one port 5683.  Things will
// get tricky if we want to listen on two ports at once though (ala DTLS)
// we may very well have to refactor all the code to use RAW/PBUF to accomplish
// that in a proper non-blocking way
// http://www.ecoscentric.com/ecospro/doc/html/ref/lwip-api-sequential-netconn-new-with-callback.html
void netconn_callback(netconn* conn, netconn_evt evt, uint16_t len)
{
    switch(evt)
    {
        case NETCONN_EVT_RCVPLUS:
            // if len > 0, then basically we want to trigger a semaphore indicating
            // data's available for this connection.  Ultimately we can hopefully
            // get rid of our timeout/blocking code that way
            break;

        default:
            break;
    }
}

void nonblocking_datapump_loop(lwip::Netconn netconn, lwip_datapump_t& datapump)
{
    addr_t addr;
    netbuf* native_netbuf;

    err_t err;

    if(!datapump.transport_empty())
    {
        lwip_datapump_t::Item& item = datapump.transport_front();
        const netbuf_t* netbuf_out = item.netbuf();
        addr = item.addr();
        uint16_t total_length = netbuf_out->length_total();

        printf("nonblocking_datapump_loop: send to port: %d / len = %d\n", 
            addr.port,
            total_length);

        native_netbuf = netbuf_out->native();

        // Note:
        // a) pbuf_realloc(native_netbuf->p) is *slightly* out of spec,
        //    but it's the only way I know to shrink-to-fit the netbuf to the right size for sending
        pbuf_realloc(native_netbuf->p, total_length);

        err = netconn.sendTo(native_netbuf, &addr.addr, addr.port);

        if(err != 0) printf("err=%d\n", err);

        // TODO: Put in IDataPumpObserver code
        
        datapump.transport_pop();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
#else
        // NOTE: Obviously not ideal and will have to change when
        // retry logic enters the picture
        delete netbuf_out;
#endif
    }

    err = netconn.recv(&native_netbuf);

    // ignore timeouts, since we expect a bunch
    if(err == ERR_TIMEOUT) return;

    // notify of other errors
    if(err != 0) 
    {
        printf("err=%d\n", err);
        return;
    }

    // initialize address with the address from incoming netbuf
    new (&addr) addr_t(native_netbuf);

    // allocate a netbuf in our own wrapper.
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    LwipNetbuf netbuf_in(native_netbuf, true);
#else
    // NOTE: Shall be expected to be explicitly deleted by
    // whatever user responder is out there when it finally
    // services the queue.  Not ideal
    LwipNetbuf& netbuf_in =  * new LwipNetbuf(native_netbuf, true);
#endif

    // 'in' netbufs always have length_processed == chunk.length
    datapump.transport_in(netbuf_in, addr);
}

}}

#endif