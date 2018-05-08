#include "coap/platform.h"

#ifdef FEATURE_MCCOAP_SOCKETS

#include "exp/datapump.hpp"
#include "../generic/malloc_netbuf.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
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


// goes into loop to listen manage datapump + sockets
// NOTE: technically with just a bit of templating this could be non-datapump specific
int blocking_datapump_handler(volatile const bool& service_active)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    if(sockfd < 0) error("ERROR opening socket");

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(COAP_UDP_PORT);

    if (bind(sockfd, (sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    while(service_active)
    {
        int newsockfd = sockfd;

        netbuf_t netbuf_in;

        uint8_t* buffer = netbuf_in.unprocessed();
        size_t buffer_len = netbuf_in.length_unprocessed();

        // Untested but should work
        // Sets 100ms timeout on recv (and technically send also)
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 ms
        if (setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
            error("Error");
        }

        //if(newsockfd < 1) error("ERROR on accept");

        ssize_t n = recvfrom(newsockfd, buffer, buffer_len, 0, (sockaddr*) &cli_addr, &clilen);

        if(n > 0)
        {
            std::clog << "Got packet: " << n << " bytes" << std::endl;

            datapump.transport_in(netbuf_in, cli_addr);
        }
        else
        {
            const netbuf_t* netbuf_out = datapump.transport_out(&cli_addr);

            if(netbuf_out != NULLPTR)
            {
                std::clog << "Responding with " << netbuf_out->length_processed() << " bytes" << std::endl;

                // FIX: Doesn't work for a multipart chunk
                sendto(newsockfd,
                       netbuf_out->processed(),
                       netbuf_out->length_processed(), 0, (sockaddr*) &cli_addr, clilen);
            }
        }
    }

    close(sockfd);

    return 0;
}

#endif
