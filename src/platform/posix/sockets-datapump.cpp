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

    const netbuf_t* netbuf_out = sockets_datapump.transport_out(&cli_addr);

    if(netbuf_out != NULLPTR)
    {
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

        // FIX: Not a long-term way to handle netbuf deallocation
        delete netbuf_out;
    }

    pollfd fd;

    fd.fd = sockfd;
    fd.events = POLL_IN; // look for incoming UDP data

    int poll_result = poll(&fd, 1, 50); // wait for 50 ms for input

    // TODO: display error if we get one
    if(poll_result <= 0) return;

    //if(newsockfd < 1) error("ERROR on accept");

    netbuf_t* netbuf_in = new netbuf_t;

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

        sockets_datapump.transport_in(*netbuf_in, cli_addr);

        // FIX: Need to find a way to gracefully deallocate netbuf in, since it's now queued
        // and needs to hang around for a bit
    }
}


int nonblocking_datapump_shutdown(int sockfd)
{
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
