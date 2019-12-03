// 12/3/2019: OBSOLETE
// We no longer use mk. 1 netbuf (which this is tailored for) and
// also there's no need to couple to our nifty but not extremely
// helpful lwip-cpp adapter.  Lastly, this would be better served
// in embr library since it's not coap specific
#pragma once

#include <platform/lwip/lwip-netbuf.h>
#include <lwip/api.hpp> // for lwip::Netconn usage

namespace moducom { namespace coap {

struct LwipPortAndAddress
{
    ip_addr_t addr;
    uint16_t port;

    LwipPortAndAddress() {}

    LwipPortAndAddress(ip_addr_t addr, uint16_t port) :
        addr(addr),
        port(port) {}

    LwipPortAndAddress(netbuf* nb)
    {
        addr = *netbuf_fromaddr(nb);
        port = netbuf_fromport(nb);
    }
};

}}
