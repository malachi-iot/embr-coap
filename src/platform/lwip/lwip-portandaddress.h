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
