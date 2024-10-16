#pragma once

//#include <embr/streambuf.h>
#include <estd/istream.h>

#include "../decoder.h"
#include "../context.h"

namespace embr { namespace coap {

// context mode is intermediate, so once native streambuf mode is fully online, zap EXPCONTEXT code
//#define FEATURE_MCCOAP_EXPCONTEXT

template <class TStreambuf>
class StreambufDecoder : public Decoder
{
    TStreambuf streambuf;
#ifdef FEATURE_MCCOAP_EXPCONTEXT
    // a bit like the netbuf-ish DecoderWithContext, but without the netbuf mk1 semantics
    // might be able to bypass context or minimize it to more stack-y behavior
    internal::DecoderContext context;
#endif

public:
    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;
    typedef estd::detail::basic_istream<streambuf_type&> istream_type;
    typedef typename streambuf_type::int_type int_type;
    typedef typename streambuf_type::size_type size_type;
    typedef typename streambuf_type::traits_type traits_type;

    // Removing CV from char_type is OK since we are always making copies of it anyhow
    // DEBT: Must assert that char_type is an 8-bit type
    typedef typename estd::remove_cv<typename traits_type::char_type>::type char_type;

    typedef typename estd::span<const char_type> span_type;

private:
#ifdef FEATURE_MCCOAP_EXPCONTEXT
    // Represents total size remaining of incoming stream/packet data to be decoded
    // NOTE: Temporary helper as we transition to truly stream-based decoding.  CoAP
    // relies heavily on the transport knowing total_size or at least a hard EOF for
    // a particular packet.  That's a little murky still with streambufs, so manually
    // specify total_size, which for every CoAP use case I can think of, is readily
    // available on a decode.  Sloppy, but not "wrong".  Eventually we want to lean more
    // on in_avail(), underflow() and friends directly in the decoder
    size_type total_size_remaining;
#endif

public:
    // Copy/adapted from decoder.h context flavor
    span_type option(bool* completed = NULLPTR);

    // FIX: temporary name until we decouple from legacy context/chunk style decoding.  Once we finish that,
    // _streambuf suffix is dropped
    // NOTE: We may find existing Decoder::process_iterate is fully sufficient in its current form, so this
    // function is considered experimental
    // NOTE: This has a diverging behavior in that it's .hpp not .cpp, so we may be facing code bloat this way
    // Without analysis, unknown if this is a real, imagined, good or bad thing
    // NOTE: pos' days are numbered, only using this for compatibility with non streambuf decoder
    // TODO: Consider passing in streambuf also and moving this into Decoder base class
    iterated::decode_result process_iterate_streambuf(size_t& pos);

    iterated::decode_result process_iterate_streambuf()
    {
#ifdef FEATURE_MCCOAP_EXPCONTEXT
        size_t& pos = context.pos;
#else
        size_t pos = 0; // dummy value, phase out this call after EXPCONTEXT is gone
#endif

        return process_iterate_streambuf(pos);
    }

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    StreambufDecoder(
#ifdef FEATURE_MCCOAP_EXPCONTEXT
            size_type total_size,
#endif
            TArgs&&... args) :
            streambuf(std::forward<TArgs>(args)...)
#ifdef FEATURE_MCCOAP_EXPCONTEXT
            // FIX: in_avail() can come back with a '0' or '-1' , still need to address that
            // FIX: Need better assessment of 'last_chunk'
            ,context(internal::const_buffer(
                    (const uint8_t*)streambuf.gptr(),
                    streambuf.in_avail()),
                    total_size <= streambuf.in_avail()),
            total_size_remaining(total_size)
#endif
    {}
#else
    template <class T1>
    StreambufDecoder(T1& arg1) :
        streambuf(arg1)
    {

    }
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

#ifdef FEATURE_MCCOAP_EXPCONTEXT
    // evaluates incoming chunk up until the next state change OR end
    // of presented chunk, then stops
    // returns true when context.chunk is fully processed, even if it is not
    // the last_chunk.  State may or may not change in this circumstance
    bool process_iterate()
    {
        bool end_reached = Decoder::process_iterate(context);

        if(end_reached)
        {
            // NOTE: expected pos is, at the end of a particular context, the same value
            // as egptr() - eback()
            // NOTE: We have to do these buffer gymnastics because streambufs don't deal in the
            // notion of absolute data input size, but the current decoder does, specifically
            // in the way it deals with 'last_chunk' (see comment on 'total_size_remaining')
            size_type chunk_size = context.pos;
            //size_type streambuf_chunk_size = streambuf.egptr() - streambuf.eback();

            // do whatever trickery needed to push streambuf forward.  If that is unsuccessful,
            // then we really do reach the end.  Still need to decide if that's *really* the end
            // or if it's a non-blocking-maybe-more-data-is-coming scenario
            int_type ch = streambuf.underflow();

            if(ch != traits_type::eof())
            {
                total_size_remaining -= chunk_size;
                size_type in_avail = streambuf.in_avail();

                // NOTE: A bit rule bending here to force a re-construction of context.  Violates
                // the spirit of the const_buffer
                // FIX: Need better assessment of 'last_chunk'
                new (&context) internal::DecoderContext(
                        internal::const_buffer(
                                (const uint8_t*)streambuf.gptr(),
                                in_avail),
                        total_size_remaining <= in_avail);
            }
        }

        return false;
    }
#endif
};

}}
