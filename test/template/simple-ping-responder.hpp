#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/decoder/subject.h>
#include <mc/memory-chunk.h>
#include <exp/datapump.h>

using namespace std;
using namespace moducom::coap;

template <class TDataPumpHelper>
void simple_ping_responder(TDataPumpHelper& sdh, typename TDataPumpHelper::datapump_t& datapump)
{
    typedef typename TDataPumpHelper::netbuf_t netbuf_t;
    typedef typename TDataPumpHelper::addr_t addr_t;

    addr_t ipaddr;
    netbuf_t* netbuf;

    netbuf = sdh.front(&ipaddr, datapump);

    // echo back out a raw ACK, with no trickery just raw decoding/encoding
    if(netbuf != NULLPTR)
    {
        //cout << " ip=" << ipaddr.sin_addr.s_addr << endl;

        NetBufDecoder<netbuf_t&> decoder(*netbuf);
        layer2::Token token;

        Header header_in = decoder.process_header_experimental();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.process_token_experimental(&token);

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        netbuf_t temporary;

        // in this scenario, netbuf gets copied around.  Ideally we'd actually do an emplace
        // but code isn't quite there yet
        netbuf = &temporary;
#else
        // FIX: Need a much more cohesive way of doing this
        delete netbuf;
        netbuf = new netbuf_t;
#endif

        sdh.pop(datapump);

        // TODO: Do away with explicit 'temporary' netbuf and make
        // it a NetBufEncoder<netbuf_t>
        NetBufEncoder<netbuf_t&> encoder(*netbuf);

        //cout << "mid out=" << header.message_id() << endl;

        encoder.header(create_response(header_in, Header::Code::Content));
        encoder.token(token);
        // optional and experimental.  Really I think we can do away with encoder.complete()
        // because coap messages are indicated complete mainly by reaching the transport packet
        // size - a mechanism which is fully outside the scope of the encoder
        encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        sdh.enqueue(std::forward<netbuf_t>(temporary), ipaddr, datapump);
#else
        sdh.enqueue(*netbuf, ipaddr, datapump);
#endif
    }
}
