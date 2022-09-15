// See README.md
#pragma once

#include <estd/span.h>
#include "../../coap/decoder/streambuf.hpp"
#include "../../coap/encoder/streambuf.h"

// Different than parent factory.h, this one focuses on specializations
// to create encoders and decoders depending on the buffers at play

namespace embr { namespace coap { namespace experimental {

template <class TBuffer>
struct DecoderFactory;

template <class TBuffer>
struct EncoderFactory;

template <class TBuffer>
struct StreambufProvider;

template <typename TChar>
struct StreambufProvider<estd::span<TChar> >
{
    typedef typename estd::remove_cv<TChar>::type char_type;

    typedef estd::internal::streambuf<estd::internal::impl::in_span_streambuf<const char_type> > istreambuf_type;
    typedef estd::internal::streambuf<estd::internal::impl::out_span_streambuf<char_type> > ostreambuf_type;
};

/*
template <>
struct DecoderFactory<estd::span<const uint8_t> >
{
    typedef estd::span<const uint8_t> buffer_type;
    typedef StreambufDecoder<streambuf_type> decoder_type;

    inline static decoder_type create(buffer_type buffer)
    {
        return decoder_type(buffer);
    }
}; */


template <class TBuffer>
struct EncoderFactory
{
    typedef TBuffer buffer_type;
    typedef typename StreambufProvider<buffer_type>::ostreambuf_type streambuf_type;
    typedef StreambufEncoder<streambuf_type> encoder_type;

#ifdef __cpp_rvalue_references
    inline static encoder_type create(buffer_type&& buffer)
    {
        return encoder_type(std::move(buffer));
    }
#endif

    inline static encoder_type create(const buffer_type& buffer)
    {
        return encoder_type(buffer);
    }
};


template <class TBuffer>
struct DecoderFactory
{
    typedef TBuffer buffer_type;
    typedef typename StreambufProvider<buffer_type>::istreambuf_type streambuf_type;
    typedef StreambufDecoder<streambuf_type> decoder_type;

#ifdef __cpp_rvalue_references
    inline static decoder_type create(buffer_type&& buffer)
    {
        return decoder_type(std::move(buffer));
    }
#endif

    inline static decoder_type create(buffer_type buffer)
    {
        return decoder_type(buffer);
    }
};

// DEBT: Helper function - somewhat problematic.  It free-floating
// out here with ADL making it consume all buffers is a little concerning.
// However, DecoderFactory specializations will only take on certain TBuffers
template <class TBuffer>
inline Header get_header(TBuffer buffer)
{
    auto decoder = DecoderFactory<TBuffer>::create(buffer);

    iterated::decode_result r;

    do
    {
        r = decoder.process_iterate_streambuf();
    }
    while(!(r.eof || decoder.state() == Decoder::HeaderDone));

    return decoder.header_decoder();
}


}}}