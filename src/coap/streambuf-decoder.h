#pragma once

#include <embr/streambuf.h>
#include <estd/istream.h>

#include "header.h"
#include "decoder.h"
#include "../coap_transmission.h"
#include "context.h"

namespace moducom { namespace coap { namespace experimental {

template <class TStreambuf>
class StreambufDecoder : public Decoder
{
    TStreambuf streambuf;
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
        context(estd::const_buffer((const uint8_t*)streambuf.gptr(), 100), true)
        {}
#endif

#ifdef FEATURE_CPP_MOVESEMANTIC
    StreambufDecoder(streambuf_type&& streambuf) :
        streambuf(std::move(streambuf)) {}
#endif

    // istream-style
    streambuf_type* rdbuf() { return &streambuf; }

    // NOTE: might prefer instead to present entire encoder as a minimal
    // (non flaggable) basic_istream.  Brings along some useful functionality
    // without adding any overhead
    istream_type istream() { return istream_type(streambuf); }

    // evaluates incoming chunk up until the next state change OR end
    // of presented chunk, then stops
    // returns true when context.chunk is fully processed, even if it is not
    // the last_chunk.  State may or may not change in this circumstance
    bool process_iterate()
    {
        bool end_reached = Decoder::process_iterate(context);

        if(end_reached)
        {
            // do whatever trickery needed to push streambuf forward.  If that is unsuccessful,
            // then we really do reach the end.  Still need to decide if that's *really* the end
            // or if it's a non-blocking-maybe-more-data-is-coming scenario
            // FIX: still needs implementation underneath
            //streambuf.underflow();
        }
        else
        {

        }

        // FIX: Not quite right, just suppressing return warning
        return end_reached;
    }
};

}}}
