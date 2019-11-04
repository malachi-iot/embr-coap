#pragma once

#include <embr/streambuf.h>
#include <estd/istream.h>

#include "header.h"
#include "decoder.h"
#include "../coap_transmission.h"
#include "context.h"

namespace moducom { namespace coap { namespace experimental {

template <class TStreambuf>
class StreambufDecoder
{
    TStreambuf streambuf;
    Decoder decoder;
    // a bit like the netbuf-ish DecoderWithContext, but without the netbuf mk1 semantics
    // might be able to bypass context or minimize it to more stack-y behavior
    internal::DecoderContext context;

public:
    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;
    typedef estd::internal::basic_istream<streambuf_type&> istream_type;
    typedef moducom::coap::experimental::layer2::OptionBase option_type;

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    StreambufDecoder(TArgs... args) :
        streambuf(std::forward<TArgs>(args)...),
        // FIX: Need proper size of incoming stream buffer
        context(estd::const_buffer(streambuf.gptr(), 10))
        {}
#endif

#ifdef FEATURE_CPP_MOVESEMANTIC
    StreambufDecoder(streambuf_type&& streambuf) :
        streambuf(std::move(streambuf)) {}
#endif
};

}}}
