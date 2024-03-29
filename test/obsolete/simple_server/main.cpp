#include <iostream>

#include "../../src/coap_transmission.h"
#include "../../src/mc/experimental.h"

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

using namespace embr::coap;



#include "../../src/coap-decoder.h"
#include "../../src/coap-encoder.h"
#include "../../src/coap-dispatcher.h"
#include "../../src/coap-token.h"
#include "../../src/mc/pipeline-reader.h"
#include "../../src/mc/pipeline-decoder.h"
#include <mc/memory-chunk.h>

#ifndef FEATURE_MCCOAP_COMPLETE_OBSERVER
#error "Requires on_complete presence"
#endif


// for setfill, setw
#include <iomanip>

std::ostream& operator <<(std::ostream& out, Header& header)
{
    // Not used, yet
    std::ios::fmtflags f(out.flags());

    out << "tkl=" << (int)header.token_length();
    out << ", mid=" << std::hex << header.message_id();
    out << ", type=" << header.type();

    const Header::Code& code = header.code_experimental();

    out << std::dec;

    out << ", code=" << code.get_class() << ".";
    // it appears setw only lasts as long as the next outgoing integer
    out << std::setfill('0') << std::setw(2) << (int)code.detail();

    // this appears to crash things
    //out.flags(f);

    return out;
}



template <class TEncoder>
void encode_response(TEncoder& encoder, const IncomingContext& context, Header::Code::Codes code)
{
    Header outgoing_header =

            create_response(context.header(), code);

    // TODO: also ifdef this out based on debug level
#if defined(FEATURE_MCCOAP_IOSTREAM_NATIVE)
    std::clog << "Sending header: " << outgoing_header << std::endl;
#endif

    encoder.header(outgoing_header);

    if (context.token_present())
        encoder.token(context.token());
}


class TestDispatcherHandler : public embr::coap::experimental::IDispatcherHandler
{
    experimental::BlockingEncoder& encoder;
    IncomingContext context;

public:
    TestDispatcherHandler(experimental::BlockingEncoder& encoder) : encoder(encoder) {}

    virtual void on_header(Header header) OVERRIDE
    {
        std::clog << "Received header: " << header << std::endl;

        context.header(header);
    }


    virtual void on_token(const moducom::pipeline::MemoryChunk::readonly_t& chunk, bool last_chunk)
    {
        std::clog << "Received token: " << chunk.length() << " bytes" << std::endl;

        context.token(&chunk);
    }

    // FIX: just crappy test code, don't do this in real life
    bool header_sent = false;

    // gets called once per discovered option, followed optionally by the
    // on_option value portion taking a pipeline message
    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {
        // as you can see we wait to send header and friends until we receive options
        // as a simulation of real life where incoming options tend to determine how
        // we respond
        if(!header_sent)
        {
            encode_response(encoder, context, Header::Code::Valid);

            encoder.payload("Response payload");

            header_sent = true;
        }

        std::cout << "Got option #: " << number << std::endl;
    }

    // will get called repeatedly until option_value is completely provided
    // Not called if option_header.length() == 0
    virtual void on_option(number_t number, const moducom::pipeline::MemoryChunk::readonly_t& option_value_part, bool last_chunk) OVERRIDE
    {
        std::string value((const char*) option_value_part.data(), option_value_part.length());

        std::cout << "Got option (" << number << ") value: " << value << std::endl;
    };


    // FIX: Not getting called, but we need some kind of "message done" signal -
    virtual void on_payload(const moducom::pipeline::MemoryChunk::readonly_t& chunk, bool last_chunk) OVERRIDE
    {
    }

#ifdef FEATURE_MCCOAP_COMPLETE_OBSERVER
    virtual void on_complete() OVERRIDE
    {
        if(!header_sent)
        {
            encode_response(encoder, context, Header::Code::NotFound);
            header_sent = true;
        }

    }
#endif

    virtual interested_t interested() const OVERRIDE
    { return interested_t::Always; }
};


int main()
{
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

        if(n <= 0) continue;

        moducom::pipeline::MemoryChunk inbuf(buffer, n);
        moducom::pipeline::layer3::MemoryChunk<256> outbuf;
        moducom::pipeline::layer3::SimpleBufferedPipelineWriter writer(outbuf);
        experimental::Dispatcher dispatcher;
        experimental::BlockingEncoder encoder(writer);

        TestDispatcherHandler handler(encoder);

        dispatcher.head(&handler);
        dispatcher.dispatch(inbuf, true);

        // FIX: only temporary as on_complete() feature still gets build out
        // eventually dispatcher will auto call this
        handler.on_complete();

        // send(...) is for connection oriented
        //send(newsockfd, outbuf._data(), outbuf._length(), 0);

        size_t send_bytes = writer.length_experimental();

        std::cout << "Responding with " << send_bytes << " bytes" << std::endl;

        sendto(newsockfd, outbuf.data(), send_bytes, 0, (sockaddr*) &cli_addr, clilen);
        //close(newsockfd);
    }

    return 0;
}