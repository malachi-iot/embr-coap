// See README.md
#pragma once

#include <estd/span.h>
#include "../../coap/decoder/streambuf.hpp"

// Different than parent factory.h, this one focuses on specializations
// to create encoders and decoders depending on the buffers at play

namespace embr { namespace coap { namespace experimental {

template <class TBuffer>
struct DecoderFactory;

template <class TBuffer>
struct EncoderFactory;

template <>
struct DecoderFactory<estd::span<const uint8_t> >
{
    typedef estd::span<const uint8_t> buffer_type;
    typedef estd::internal::streambuf<estd::internal::impl::in_span_streambuf<const uint8_t> > streambuf_type;
    typedef StreambufDecoder<streambuf_type> decoder_type;

    inline static decoder_type create(buffer_type buffer)
    {
        return decoder_type(buffer);
    }
};

}}}