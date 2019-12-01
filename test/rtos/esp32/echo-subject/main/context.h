#pragma once

#include <coap/context.h>
#include <embr/platform/lwip/pbuf.h>

struct AppContext : moducom::coap::IncomingContext<ip_addr_t>
{
    
};