#pragma once

#include "exp/datapump.h"
#include <netinet/in.h>
#include "../generic/malloc_netbuf.h"

namespace moducom { namespace coap {

typedef moducom::coap::experimental::DataPump<moducom::coap::NetBufDynamicExperimental, sockaddr_in> sockets_datapump_t;

extern sockets_datapump_t sockets_datapump;

}}

int nonblocking_datapump_setup();
void nonblocking_datapump_loop(int h, moducom::coap::sockets_datapump_t& datapump = moducom::coap::sockets_datapump);
int nonblocking_datapump_shutdown(int);

namespace moducom { namespace coap {

// combining socket handle with queue allocation
// not 100% sure we always want to do that, but seems good so far
class SocketsDatapumpHelper
{
    typedef sockets_datapump_t::addr_t addr_t;
    typedef sockets_datapump_t::netbuf_t netbuf_t;

    int sockfd;

public:
    SocketsDatapumpHelper()
    {
        sockfd = nonblocking_datapump_setup();
    }

    ~SocketsDatapumpHelper()
    {
        nonblocking_datapump_shutdown(sockfd);
    }

    void loop(sockets_datapump_t& datapump = sockets_datapump)
    {
        // TODO: make nonblocking_datapump_loop into something that accepts
        // datapump parameter
        nonblocking_datapump_loop(sockfd, datapump);
    }

    // dequeue input that was previously queued up from transport
    // returns NULL if no item queued
    netbuf_t* dequeue(addr_t* addr_in, sockets_datapump_t& datapump = sockets_datapump)
    {
        return datapump.dequeue_in(addr_in);
    }

    // queue up to send out over transport
    void enqueue(netbuf_t& netbuf, const addr_t& addr_out, sockets_datapump_t& datapump = sockets_datapump)
    {
        datapump.enqueue_out(netbuf, addr_out);
    }
};

}}
