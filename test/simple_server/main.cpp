#include <iostream>

#include "../../src/coap_transmission.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define V2

// bits adapted from http://www.linuxhowtos.org/data/6/server.c
// and https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

using namespace moducom::coap;



#ifdef V1

class TestResponder : public CoAP::IResponder
{
public:
    virtual void OnHeader(const CoAP::Header header) OVERRIDE;
    virtual void OnToken(const uint8_t message[], size_t length) OVERRIDE;
    virtual void OnOption(const CoAP::OptionExperimental& option) OVERRIDE;
    virtual void OnPayload(const uint8_t message[], size_t length) OVERRIDE;
    virtual void OnCompleted() OVERRIDE;
};

void TestResponder::OnHeader(const CoAP::Header header) {}

void TestResponder::OnOption(const CoAP::OptionExperimental &option) {}

void TestResponder::OnPayload(const uint8_t *message, size_t length) {}

void TestResponder::OnToken(const uint8_t *message, size_t length) {}

void TestResponder::OnCompleted()
{
    std::cout << "Finished CoAP processing" << std::endl;
}
#else
#include "../../src/coap-decoder.h"
#include "../../src/coap-encoder.h"
#include "../../src/coap-dispatcher.h"
#include "../../src/coap-token.h"
#include "../../src/mc/pipeline-reader.h"
#include "../../src/mc/pipeline-decoder.h"
#include "../../src/mc/memory-chunk.h"

class TestDispatcherHandler : public moducom::coap::experimental::IDispatcherHandler
{
    experimental::BlockingEncoder& encoder;

public:
    TestDispatcherHandler(experimental::BlockingEncoder& encoder) : encoder(encoder) {}

    virtual void on_header(CoAP::Header header) OVERRIDE
    {
        CoAP::Header::TypeEnum type = header.type();
        uint8_t token_length = header.token_length();
        uint16_t mid = header.message_id();

        ASSERT_ERROR(CoAP::Header::Confirmable, type, "Expected confirmable");

        // assuming we were requested with confirmable
        CoAP::Header outgoing_header(CoAP::Header::Acknowledgement);

        outgoing_header.response_code(CoAP::Header::Code::Valid);
        outgoing_header.token_length(token_length);
        outgoing_header.message_id(mid);

        encoder.header(outgoing_header);
    }


    virtual void on_token(const moducom::pipeline::MemoryChunk& chunk, bool last_chunk)
    {
        encoder.token(chunk);
    }


    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {
        encoder.payload("Response payload");
    }

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(const moducom::pipeline::MemoryChunk& option_value_part, bool last_chunk)
    {
    };


    virtual void on_payload(const moducom::pipeline::MemoryChunk& chunk, bool last_chunk) OVERRIDE
    {

    }

    virtual InterestedEnum interested() OVERRIDE { return Always; }
};

#endif

int main()
{
#ifdef V1
    //moducom::coap::CoAP::Header header;
    experimental::OutgoingResponseHandler out_handler(nullptr);
    experimental::TestResponder responder;
    TestResponder user_responder;
    CoAP::Parser parser;

    responder.add_handle("basic/", &user_responder);
#else
#endif

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

        //if(newsockfd < 1) error("ERROR on accept");

        bzero(buffer, 1024);

        //size_t n = read(newsockfd, buffer, 1024);
        ssize_t n = recvfrom(newsockfd, buffer, sizeof(buffer), 0, (sockaddr*) &cli_addr, &clilen);

        std::cout << "Got packet: " << n << " bytes" << std::endl;

#ifdef V1
        CoAP::ParseToIResponder parser2(&responder);

        if(n > 0)
            parser2.process(buffer, n);
#else
        if(n <= 0) continue;

        moducom::pipeline::MemoryChunk inbuf(buffer, n);
        moducom::pipeline::layer3::MemoryChunk<256> outbuf;
        moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
        experimental::Dispatcher dispatcher;
        experimental::BlockingEncoder encoder(writer);

        TestDispatcherHandler handler(encoder);

        dispatcher.head(&handler);
        dispatcher.dispatch(inbuf, true);

        // send(...) is for connection oriented
        //send(newsockfd, outbuf._data(), outbuf._length(), 0);

        size_t send_bytes = writer.length_experimental();

        std::cout << "Responding with " << send_bytes << " bytes" << std::endl;

        sendto(newsockfd, outbuf._data(), send_bytes, 0, (sockaddr*) &cli_addr, clilen);
#endif
        //close(newsockfd);
    }

    return 0;
}