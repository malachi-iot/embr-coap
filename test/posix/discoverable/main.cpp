#include <iostream>

#include <platform/posix/sockets-datapump.h>

#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>

using namespace moducom::coap;

int main()
{
    std::cout << "CoRE test" << std::endl;

    SocketsDatapumpHelper sdh;

    for(;;)
    {
        sdh.loop(sockets_datapump);

        auto& datapump = sockets_datapump;

        typedef sockets_datapump_t datapump_t;
        typedef typename datapump_t::netbuf_t netbuf_t;
        typedef typename datapump_t::addr_t addr_t;
        typedef typename datapump_t::Item item_t;
        typedef NetBufDecoder<netbuf_t&> decoder_t;

        if(!datapump.dequeue_empty())
        {
            item_t& item = datapump.dequeue_front();

            // hold on to ipaddr for when we enqueue a response
            addr_t ipaddr = item.addr();

            //decoder_t decoder(*sdh.front(&ipaddr, datapump));
            decoder_t decoder(*item.netbuf());

            Header header_in = decoder.header();

            layer2::Token token;

            // populate token, if present.  Expects decoder to be at HeaderDone phase
            decoder.token(&token);

            // skip any options
            option_iterator<decoder_t> it(decoder, true);

            while(it.valid()) ++it;

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            NetBufEncoder<netbuf_t> encoder;
#else
            // FIX: Need a much more cohesive way of doing this
            delete &decoder.netbuf();

            NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

            datapump.dequeue_pop();

            encoder.header(create_response(header_in, Header::Code::Content));
            encoder.token(token);
            // optional and experimental.  Really I think we can do away with encoder.complete()
            // because coap messages are indicated complete mainly by reaching the transport packet
            // size - a mechanism which is fully outside the scope of the encoder
            encoder.complete();

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.enqueue_out(std::move(encoder.netbuf()), ipaddr);
#else
            datapump.enqueue_out(encoder.netbuf(), ipaddr);
#endif
        }
    }

    return 0;
}