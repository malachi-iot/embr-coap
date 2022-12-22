#pragma once

#include <estd/streambuf.h>

#include <coap/context.h>
#include <coap/encoder/streambuf.h>

typedef estd::internal::streambuf<
    estd::internal::impl::out_span_streambuf<char> > out_span_streambuf_type;
typedef embr::coap::StreambufEncoder<out_span_streambuf_type> span_encoder_type;


// NOTE: If experimental transport stuff from embr comes together, this and other incoming context
// will become more organized
struct SyntheticIncomingContext :
    embr::coap::IncomingContext<unsigned>,
    embr::coap::UriParserContext
{
    typedef embr::coap::IncomingContext<unsigned> base_type;
    typedef span_encoder_type encoder_type;

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
