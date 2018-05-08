#pragma once

#include "exp/datapump.h"
#include <netinet/in.h>
#include "../generic/malloc_netbuf.h"

int nonblocking_datapump_setup();
void nonblocking_datapump_loop(int);
int nonblocking_datapump_shutdown(int);

namespace moducom { namespace coap {

typedef moducom::coap::experimental::DataPump<moducom::coap::NetBufDynamicExperimental, sockaddr_in> sockets_datapump_t;

extern sockets_datapump_t sockets_datapump;

}}
