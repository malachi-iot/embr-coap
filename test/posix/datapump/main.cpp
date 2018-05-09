#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder-subject.h>
#include <mc/memory-chunk.h>

using namespace std;
using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    cout << "Hello World!" << endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop();

        sockaddr_in ipaddr;
        netbuf_t* netbuf;

        netbuf = sdh.dequeue(&ipaddr);

        // echo back out a raw ACK, with no trickery just raw decoding/encoding
        if(netbuf != NULLPTR)
        {
            //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

            NetBufDecoder<netbuf_t&> decoder(*netbuf);
            layer2::Token token;

            Header header_in = decoder.process_header_experimental();

            // populate token, if present.  Expects decoder to be at HeaderDone phase
            decoder.process_token_experimental(&token);

            // FIX: Need a much more cohesive way of doing this
            delete netbuf;

            netbuf = new netbuf_t;

            NetBufEncoder<netbuf_t&> encoder(*netbuf);

            //cout << "mid out=" << header.message_id() << endl;

            encoder.header(create_response(header_in, Header::Code::Content));
            encoder.token(token);
            // optional and experimental.  Really I think we can do away with encoder.complete()
            // because coap messages are indicated complete mainly by reaching the transport packet
            // size - a mechanism which is fully outside the scope of the encoder
            encoder.complete();

            sdh.enqueue(*netbuf, ipaddr);
        }
    }

    return 0;
}
