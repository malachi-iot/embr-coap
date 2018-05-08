#include "coap/platform.h"

#ifdef FEATURE_MCCOAP_SOCKETS

#include "exp/datapump.hpp"
#include "../generic/malloc_netbuf.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

static moducom::coap::experimental::DataPump<netbuf_t, sockaddr_in> datapump;

#define COAP_UDP_PORT 5683

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int nonblocking_datapump_setup()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serv_addr;

    if(sockfd < 0) error("ERROR opening socket");

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(COAP_UDP_PORT);

    if (bind(sockfd, (sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    return sockfd;
}


void nonblocking_datapump_loop(int sockfd)
{
    sockaddr_in cli_addr;
    socklen_t clilen;

    const netbuf_t* netbuf_out = datapump.transport_out(&cli_addr);

    if(netbuf_out != NULLPTR)
    {
        std::clog << "Responding with " << netbuf_out->length_processed() << " bytes" << std::endl;

        // FIX: Doesn't work for a multipart chunk
        sendto(sockfd,
               netbuf_out->processed(),
               netbuf_out->length_processed(), 0, (sockaddr*) &cli_addr, clilen);
    }

    pollfd fd;

    fd.fd = sockfd;
    fd.events = POLL_IN; // look for incoming UDP data

    int poll_result = poll(&fd, 1, 100); // wait for 100 ms for input

    // TODO: display error if we get one
    if(poll_result <= 0) return;

    //if(newsockfd < 1) error("ERROR on accept");

    netbuf_t* netbuf_in = new netbuf_t;

    uint8_t* buffer = netbuf_in->unprocessed();
    size_t buffer_len = netbuf_in->length_unprocessed();

    ssize_t n = recvfrom(sockfd, buffer, buffer_len, 0, (sockaddr*) &cli_addr, &clilen);

    if(n > 0)
    {
        std::clog << "Got packet: " << n << " bytes" << std::endl;

        datapump.transport_in(*netbuf_in, cli_addr);

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
