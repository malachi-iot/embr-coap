#pragma once

#include <estd/streambuf.h>

#include <coap/context.h>
#include <coap/encoder/streambuf.h>

namespace test {

extern char synthetic_outbuf[128];

}

typedef estd::internal::streambuf<
    estd::internal::impl::out_span_streambuf<char> > out_span_streambuf_type;
typedef embr::coap::StreambufEncoder<out_span_streambuf_type> span_encoder_type;

struct SyntheticBufferFactory
{
    // DEBT: I'd like us to be able to specify 128 on span here,
    // but conversions aren't quite working for that
    static estd::span<char> create() { return test::synthetic_outbuf; }
};


// NOTE: If experimental transport stuff from embr comes together, this and other incoming context
// will become more organized
struct SyntheticIncomingContext :
    embr::coap::IncomingContext<unsigned>,
    embr::coap::UriParserContext
{
    typedef embr::coap::IncomingContext<unsigned> base_type;
    typedef span_encoder_type encoder_type;
    typedef embr::coap::experimental::EncoderFactory<
        estd::span<char>,
        SyntheticBufferFactory> encoder_factory;

    estd::span<char> out;

    void reply(encoder_type& encoder)
    {
        encoder.finalize();
        out = encoder.rdbuf()->value();
    }

    template <int N>
    SyntheticIncomingContext(const UriPathMap (&paths)[N]) :
        embr::coap::UriParserContext(paths) {}
};

struct ExtraContext : SyntheticIncomingContext,
                      embr::coap::internal::ExtraContext
{
    void reply(encoder_type& encoder)
    {
        SyntheticIncomingContext::reply(encoder);
        flags.response_sent = true;
    }

    template <int N>
    ExtraContext(const UriPathMap (&paths)[N]) :
        SyntheticIncomingContext(paths) {}
};
