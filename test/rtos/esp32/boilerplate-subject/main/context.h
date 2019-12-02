#pragma once

#include "lwip-context.h"

struct AppContext : 
    moducom::coap::IncomingContext<const ip_addr_t*>,
    LwipContext
{
    AppContext(struct udp_pcb* pcb, 
        const ip_addr_t* addr,
        uint16_t port) : 
        LwipContext(pcb, port)
    {
        this->addr = addr;
    }
};