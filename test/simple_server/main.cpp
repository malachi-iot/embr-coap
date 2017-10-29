#include <iostream>

#include "../../src/coap_transmission.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// bits adapted from http://www.linuxhowtos.org/data/6/server.c

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

using namespace moducom::coap;

int main()
{
    //moducom::coap::CoAP::Header header;
    experimental::OutgoingResponseHandler out_handler(nullptr);
    experimental::TestResponder responder;
    CoAP::Parser parser;

    std::cout << "Hello, World!" << std::endl;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    uint8_t buffer[1024];

    if(sockfd < 0) error("ERROR opening socket");

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5683);

    if (bind(sockfd, (sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // listen only for TCP it seems
    //listen(sockfd, 5);

    for(;;)
    {
        // accept only for TCP it seems
        //int newsockfd = accept(sockfd, (sockaddr *) &cli_addr, &clilen);
        int newsockfd = sockfd;

        if(newsockfd < 1) error("ERROR on accept");

        bzero(buffer, 1024);

        //size_t n = read(newsockfd, buffer, 1024);
        size_t n = recvfrom(newsockfd, buffer, sizeof(buffer), 0, (sockaddr*) &cli_addr, &clilen);

        std::cout << "Got packet: " << n << " bytes" << std::endl;

        CoAP::ParseToIResponder parser2(&responder);

        if(n > 0)
            parser2.process(buffer, n);

        close(newsockfd);
    }

    return 0;
}