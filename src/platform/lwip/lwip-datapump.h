#pragma once

#include <platform/lwip/lwip-netbuf.h>
#include "exp/datapump.h"

namespace moducom { namespace coap {

struct LwipPortAndAddress
{
    ip_addr_t addr;
    uint16_t port;

    LwipPortAndAddress() {}

    LwipPortAndAddress(netbuf* nb)
    {
        addr = *netbuf_fromaddr(nb);
        port = netbuf_fromport(nb);
    }
};

typedef moducom::coap::experimental::DataPump<moducom::coap::LwipNetbuf, LwipPortAndAddress> lwip_datapump_t;


}}