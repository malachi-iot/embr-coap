// See README.md
#pragma once

#include <estd/span.h>
#include <estd/variant.h>
#include <estd/streambuf.h>

#include "../fwd.h"

// Different than encoder factory.h, this one focuses on specializations
// to create encoders and decoders depending on the buffers at play

namespace embr { namespace coap { namespace internal {

template <class TBuffer>
struct DecoderFactory;

template <class TBuffer, class TBufferFactory = estd::monostate>
struct EncoderFactory;

// DEBT: This particular one wants to live in embr
template <class TBuffer>
struct StreambufProvider;

// DEBT: This is more of a diagnostic one, likely doesn't belong in production headers
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


template <class TBuffer, class TBufferFactory>
struct EncoderFactory
{
    typedef TBuffer buffer_type;
    typedef typename StreambufProvider<buffer_type>::ostreambuf_type streambuf_type;
    typedef StreambufEncoder<streambuf_type> encoder_type;
    typedef TBufferFactory buffer_factory;

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

    inline static encoder_type create()
    {
        return encoder_type(buffer_factory{}.create());
    }

    inline static encoder_type create(const buffer_factory& bf)
    {
        return encoder_type(bf.create());
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

}}}