#include <iostream>

#include <platform/posix/sockets-datapump.h>
#include <coap/encoder.h>
#include <coap/decoder-subject.h>
#include <mc/memory-chunk.h>

using namespace std;
using namespace moducom::coap;

typedef NetBufDynamicExperimental netbuf_t;

int main()
{
    cout << "Hello World!" << endl;

    int handle = nonblocking_datapump_setup();

    for(;;)
    {
        nonblocking_datapump_loop(handle);
        sockaddr_in ipaddr;
        netbuf_t* netbuf;

        netbuf = sockets_datapump.dequeue_in(&ipaddr);

        // echo back out a raw ACK, with no trickery just raw decoding/encoding
        if(netbuf != NULLPTR)
        {
            cout << "Got a netbuf";
            //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

            Decoder decoder;

            moducom::pipeline::MemoryChunk::readonly_t chunk(netbuf->processed(), netbuf->length_processed());

            Decoder::Context context(chunk, true);

            decoder.process_iterate(context);
            decoder.process_iterate(context);

            ASSERT_ERROR(Decoder::HeaderDone, decoder.state(), "Unexpected state");

            Header header_in = decoder.header_decoder();
            int tkl = header_in.token_length();
            layer1::Token token;

            //cout << "mid=" << header_in.message_id() << endl;

            if(tkl > 0)
            {
                decoder.process_iterate(context);
                decoder.process_iterate(context);

                ASSERT_ERROR(Decoder::TokenDone, decoder.state(), "Unexpected state");

                token.memcpy(decoder.token_decoder().data(), header_in.token_length());
            }

            // FIX: Need a much more cohesive way of doing this
            delete netbuf;

            netbuf = new netbuf_t;

            NetBufEncoder<netbuf_t&> encoder(*netbuf);

            Header header(Header::TypeEnum::Acknowledgement);

            header.type(Header::TypeEnum::Acknowledgement);
            header.code(Header::Code::Content);
            header.message_id(header_in.message_id());
            header.token_length(tkl);

            //cout << "mid out=" << header.message_id() << endl;

            encoder.header(header);
            encoder.token(token.data(), tkl);
            encoder.complete();

            sockets_datapump.enqueue_out(*netbuf, ipaddr);
        }
    }

    nonblocking_datapump_shutdown(handle);

    return 0;
}
