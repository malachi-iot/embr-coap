#pragma once

#include <estd/string.h>
#include <coap/platform.h>
#include <coap/encoder.h>
#include <coap/decoder.h>
#include <coap/context.h>
#include <mc/memory-chunk.h>


template <class TNetBuf, bool inline_token>
void encode_header_and_token(moducom::coap::NetBufEncoder<TNetBuf>& encoder,
                             moducom::coap::TokenAndHeaderContext<inline_token>& context,
                             moducom::coap::Header::Code::Codes response_code)
{
    encoder.header(moducom::coap::create_response(context.header(),
                                   response_code));
    encoder.token(context.token());
}


namespace moducom { namespace coap {

// Neat, but right way to do this would be to make a 'super' OptionsDecoder which had a bit
// of the start-stop condition awareness of full Decoder, then actually derive from that class
// so that we can do things like postfix++
template <class TNetBuf>
class experimental_option_iterator
{
    typedef NetBufDecoder<TNetBuf> decoder_t;
    typedef Option::Numbers value_type;

    decoder_t& decoder;
    Option::Numbers number;

    void partial_advance_and_get_number()
    {
        using namespace moducom::coap;

        decoder.option_experimental(&number);
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << "Option: ";// << number;
        ::operator <<(std::clog, number); // why do I have to do this??
#endif
    }

public:
    experimental_option_iterator(decoder_t& decoder, bool begin_option = false) :
        decoder(decoder)
    {
        number = Option::Zeroed;
        // NOT repeatable
        if(begin_option)
            decoder.begin_option_experimental();

        partial_advance_and_get_number();
    }


    bool valid() const
    {
        return decoder.state() == Decoder::Options;
    }

    experimental_option_iterator& operator ++()
    {
        decoder.option_next_experimental();
        if(valid())
        {
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif
            partial_advance_and_get_number();
        }
        else
        {
#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif
        }

        return *this;
    }

    /* disabling postfix version because things like basic_string would be out of sync.
     * could also be problematic with chunked netbufs
    experimental_option_iterator operator ++(int)
    {
        experimental_option_iterator temp(decoder, number);
        operator ++();
        return temp;
    } */

    operator const value_type&()
    {
        return number;
    }

    estd::layer3::basic_string<const char, false> string()
    {
        using namespace moducom::coap;

        estd::layer3::basic_string<const char, false> s = decoder.option_string_experimental();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
        std::clog << " (";
        ::operator <<(std::clog, s);
        std::clog << ')';
#endif
        return s;
    }

};

}}


template <class TDataPump>
void simple_uri_responder2(TDataPump& datapump, typename TDataPump::IncomingContext& context)
{
    using namespace moducom::coap;

    typedef typename TDataPump::netbuf_t netbuf_t;

    estd::layer1::string<128> uri;
    Header::Code::Codes response_code = Header::Code::NotFound;
    experimental_option_iterator<netbuf_t&> it(context.decoder());

    while(it.valid())
    {
        switch(it)
        {
            case  Option::UriPath:
            {
                uri += it.string();
                uri += '/';
                break;
            }

            default: break;
        }

        // FIX: Not accounting for chunking - in that case
        // we would need multiple calls to option_string_experimental()
        // before calling next
        ++it;
    }

    if(uri == "test/") response_code = Header::Code::Valid;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    datapump.dequeue_pop();
    NetBufEncoder<netbuf_t> encoder;
#else
    // FIX: Need a much more cohesive way of doing this
    delete &decoder.netbuf();
    datapump.dequeue_pop();

    NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

    encode_header_and_token(encoder, context, response_code);

    // optional and experimental.  Really I think we can do away with encoder.complete()
    // because coap messages are indicated complete mainly by reaching the transport packet
    // size - a mechanism which is fully outside the scope of the encoder
    encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    datapump.enqueue_out(std::forward<netbuf_t>(encoder.netbuf()), context.address());
#else
    datapump.enqueue_out(encoder.netbuf(), ipaddr);
#endif
}

template <class TDataPumpHelper>
void simple_uri_responder(TDataPumpHelper& dh, typename TDataPumpHelper::datapump_t& datapump)
{
    using namespace moducom::coap;

    typedef typename TDataPumpHelper::netbuf_t netbuf_t;
    typedef typename TDataPumpHelper::addr_t addr_t;
    typedef typename moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;

    addr_t ipaddr;

    if(!dh.empty(datapump))
    {
        NetBufDecoder<netbuf_t&> decoder(*dh.front(&ipaddr, datapump));
        layer2::Token token;
        bool gotit = false;
        estd::layer1::string<128> uri;

        Header header_in = decoder.process_header_experimental();

        // populate token, if present.  Expects decoder to be at HeaderDone phase
        decoder.process_token_experimental(&token);

        decoder.begin_option_experimental();

        // NOTE: Needs to be out here like this, since CoAP presents option numbers
        // as deltas
        Option::Numbers number = Option::Zeroed;

        while(decoder.state() == Decoder::Options)
        {
            decoder.option_experimental(&number);

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << "Option: " << number;
#endif

            if(number == Option::UriPath)
            {
                const estd::layer3::basic_string<char, false> s =
                        decoder.option_string_experimental();

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
                std::clog << " (" << s << ')';
#endif

                gotit = s == "test";

                if(uri.size() == 0)
                    uri = s;
                else
                {
                    uri += '/';
                    uri += s;
                }
            }

#ifdef FEATURE_ESTD_IOSTREAM_NATIVE
            std::clog << std::endl;
#endif

            // FIX: Not accounting for chunking - in that case
            // we would need multiple calls to option_string_experimental()
            // before calling next
            decoder.option_next_experimental();
        }

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        NetBufEncoder<netbuf_t> encoder;
#else
        // FIX: Need a much more cohesive way of doing this
        delete &decoder.netbuf();

        NetBufEncoder<netbuf_t&> encoder(* new netbuf_t);
#endif

        dh.pop(datapump);

        encoder.header(create_response(header_in,
                                       gotit ? Header::Code::Content : Header::Code::NotFound));
        encoder.token(token);
        // optional and experimental.  Really I think we can do away with encoder.complete()
        // because coap messages are indicated complete mainly by reaching the transport packet
        // size - a mechanism which is fully outside the scope of the encoder
        encoder.complete();

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        dh.enqueue(std::forward<netbuf_t>(encoder.netbuf()), ipaddr, datapump);
#else
        dh.enqueue(encoder.netbuf(), ipaddr, datapump);
#endif
    }
}
