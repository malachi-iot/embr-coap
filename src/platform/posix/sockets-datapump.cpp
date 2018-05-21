#include "../../coap/platform.h"

#ifdef FEATURE_MCCOAP_SOCKETS

#include "exp/datapump.hpp"
#include "sockets-datapump.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h> // for POLL_IN

using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

namespace moducom { namespace coap {

sockets_datapump_t sockets_datapump;

}}

#define COAP_UDP_PORT 5683

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

static int initialize_udp_socket(int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serv_addr;

    if(sockfd < 0) error("ERROR opening socket");

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    return sockfd;
}

int nonblocking_datapump_setup()
{
    return initialize_udp_socket(COAP_UDP_PORT);
}


void nonblocking_datapump_loop(int sockfd, sockets_datapump_t& sockets_datapump)
{
    sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    ssize_t n;
    typedef sockets_datapump_t::Item item_t;

    if(!sockets_datapump.transport_empty())
    {
        item_t& item = sockets_datapump.transport_front();

        cli_addr = item.addr();

        item.on_message_transmitting();

        netbuf_t* netbuf_out = item.netbuf(); // an item *always* has a valid netbuf at this point

        std::clog << "Responding with " << netbuf_out->length_processed() << " bytes";
        //std::clog << " to ip=" << cli_addr.sin_addr.s_addr << ", port=" << cli_addr.sin_port;
        std::clog << std::endl;

        const void* processed = netbuf_out->processed();
        n = netbuf_out->length_processed();

        // FIX: Doesn't work for a multipart chunk
        n = sendto(sockfd,
               processed,
               n, 0, (sockaddr*) &cli_addr, clilen);

        if(n == -1) error("Couldn't send UDP");

#ifdef FEATURE_MCCOAP_RELIABLE
        // TODO: evaluate Item.m_retry.retransmission_counter and if it's < 4
        // then don't deallocate netbuf yet.  Instead, move the netbuf to the
        // retry list via some form of retry.enqueue
        // Likely will want the architectual changes:
        // a) return Item& from transport_front
        // b) because of a), lean more heavily on transport_empty
        // c) Retry is going to *want* to be embedded in DataPump, but if we can
        //    resist that, then datapump can probably stay coap and transport agnostic
        // c.1) it's conceivable to make Retry itself coap and transport agnostic, but
        //      that would require a fair bit of work.  So, in the short term, see
        //      if we can merely make it a mechanism who behaviorally (but not data-wise)
        //      is decoupled from DataPump
        bool netbuf_ownership_transferred = item.on_message_transmitted();
#else
        bool netbuf_ownership_transferred = false; // always own netbuf in this scenario
#endif

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
#else
        // FIX: Not a long-term way to handle netbuf deallocation
        if(!netbuf_ownership_transferred)
            delete netbuf_out;
#endif

        sockets_datapump.transport_pop();
    }

    pollfd fd;

    fd.fd = sockfd;
    fd.events = POLL_IN; // look for incoming UDP data

    int poll_result = poll(&fd, 1, 50); // wait for 50 ms for input

    // This is a poll timeout, we expect this
    if(poll_result == 0) return;

    if(poll_result == -1)
        error("ERROR on poll");

    if(poll_result != 1)
        std::clog << "Warning: got unexpected poll_result: " << poll_result;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    netbuf_t temporary;
    // in this scenario, netbuf_in gets value-copied into the queue
    netbuf_t* netbuf_in = &temporary;
#else
    netbuf_t* netbuf_in = new netbuf_t;
#endif

    uint8_t* buffer = netbuf_in->unprocessed();
    size_t buffer_len = netbuf_in->length_unprocessed();

    n = recvfrom(sockfd, buffer, buffer_len, 0, (sockaddr*) &cli_addr, &clilen);

    if(n > 0)
    {
        std::clog << "Got packet: " << n << " bytes";
        //std::clog << " ip=" << cli_addr.sin_addr.s_addr;
        //std::clog << " port=" << cli_addr.sin_port;
        std::clog << std::endl;

        netbuf_in->advance(n);

        sockets_datapump.transport_in(
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
                    std::forward<netbuf_t>(temporary),
#else
                    *netbuf_in,
#endif
                    cli_addr);
    }
    else
    {
        error("ERROR on recvfrom");
    }
}


int nonblocking_datapump_shutdown(int sockfd)
{
    std::clog << "Closing socket: " << sockfd;

    close(sockfd);

    return 0;
}

// goes into loop to listen manage datapump + sockets
// NOTE: technically with just a bit of templating this could be non-datapump specific
int blocking_datapump_handler(volatile const bool& service_active)
{
    int sockfd = nonblocking_datapump_setup();

    while(service_active)
        nonblocking_datapump_loop(sockfd);

    return nonblocking_datapump_shutdown(sockfd);
}

#endif
