#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.hpp>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>

#include <arpa/inet.h> // for inet_addr

using namespace std;
using namespace moducom::coap;

#define COAP_PORT 5683

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    const char* COAP_SERVER = "192.168.2.3";
    cout << "Requesting from " << COAP_SERVER << endl;

    SocketsDatapumpHelper sdh;

    // construct a confirmable ping
    NetBufEncoder<netbuf_t> encoder;

    estd::layer2::const_string uri = "batt-level";

    uint8_t raw_token[] = { 0x1, 0x2, 0x3 };

    Header h(Header::Confirmable);

    h.code(Header::Code::Get);
    h.message_id(0x123);
    h.token_length(sizeof(raw_token));

    // TODO: Might be able to check for malformed header at this point (with debug defines = on)
    encoder.header(h);
    encoder.token(raw_token, sizeof(raw_token));
    encoder.option(Option::UriPath, uri);
    encoder.complete();

    sockaddr_in addr_out;

    addr_out.sin_family = AF_INET;
    //addr_out.sin_port = COAP_PORT;
    addr_out.sin_port = htons(COAP_PORT);
    addr_out.sin_addr.s_addr = inet_addr(COAP_SERVER);

    sdh.enqueue(std::forward<netbuf_t>(encoder.netbuf()), addr_out);

    for(;;)
    {
        sdh.loop();

        if(!sdh.empty())
        {
            sockaddr_in addr_in;

            netbuf_t* response = sdh.front(&addr_in);
            NetBufDecoder<netbuf_t&> decoder(*response);

            decoder.header();
            decoder.process_token_experimental();

            option_iterator<netbuf_t> it(decoder);

            while(it.valid())
            {
                ++it;
            }

            // TODO: extract payload
            //const char* payload = (const char*)decoder.netbuf().unprocessed();

            break;
        }
    }

    return 0;
}
