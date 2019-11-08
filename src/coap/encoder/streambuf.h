/**
 * @file
 */
#pragma once

#include <embr/streambuf.h>
#include <estd/streambuf.h>
#include <estd/ostream.h>

#include "../header.h"
#include "../../coap-encoder.h"
#include "../../coap_transmission.h"
#include "../context.h"

// used 'embr' here since we might consider phasing out 'moducom' namespace
namespace embr { namespace coap { namespace experimental {

// experimental replacement for netbuf encoder.  it's possible streambufs
// are an all around replacement for netbufs, or at least most of the time
// TODO: Still need to address partial (chunked) writes
// NOTE: header and token no chunking is not planned, we'd have to employ HeaderEncoder and
// TokenEncoder.  That use case is an edge case, since maximum size of header + token = 12 bytes
template <class TStreambuf>
class StreambufEncoder
{
    typedef moducom::coap::internal::Root::State state_type;

#ifdef DEBUG
    void _assert(state_type s)
    {
        // TODO:
    }
#else
    void _assert(state_type s) {}
    /// \brief Only used for debug purposes.  noop in release mode
    /// \param s
    void state(state_type s) {}
#endif

    typedef StreambufEncoder<TStreambuf> this_type;

public:
    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;
    typedef estd::internal::basic_ostream<streambuf_type&> ostream_type;
    typedef moducom::coap::experimental::layer2::OptionBase option_type;

protected:
    typedef typename streambuf_type::char_type char_type;
    typedef typename streambuf_type::traits_type traits_type;

    TStreambuf streambuf;
    moducom::coap::OptionEncoder option_encoder;

    estd::streamsize sputn(const uint8_t* bytes, estd::streamsize n)
    {
        return rdbuf()->sputn(reinterpret_cast<const char_type*>(bytes), n);
    }

    estd::streamsize remaining()
    {
        return rdbuf()->epptr() - rdbuf()->pptr();
    }

    // returns true when a byte is generated
    // false when state machine is processing (inspect
    // state at this time)
    bool option_generate_byte()
    {
        int result;
        result = option_encoder.generate_iterate();

        if(result != option_encoder.signal_continue)
            rdbuf()->sputc(result);

        return result != option_encoder.signal_continue;
    }

public:
    StreambufEncoder(const estd::span<uint8_t>& span) : streambuf(span) {}

    // ostream-style
    streambuf_type* rdbuf() { return &streambuf; }

    // NOTE: might prefer instead to present entire encoder as a minimal
    // (non flaggable) basic_ostream.  Brings along some useful functionality
    // without adding any overhead
    ostream_type ostream() { return ostream_type(streambuf); }

    void header(const moducom::coap::Header& header)
    {
        _assert(state_type::Uninitialized);
        // TODO: Chunked not handled, but we could still check for fail to write
        sputn(header.bytes, 4);
        state(state_type::HeaderDone);
    }


    // NOTE: token size must have already been specified in header!
    void token(const estd::const_buffer token)
    {
        // TODO: Chunked not handled, but we could still check for fail to write
        sputn(token.data(), token.size());
    }


    template <bool inline_token, bool simple_buffer>
    void header_and_token(const moducom::coap::TokenAndHeaderContext<inline_token, simple_buffer>&
    ctx)
    {
        header(ctx.header());
        token(ctx.token());
    }

    // returns false if chunking is needed
    // returns true if entire data was output
    // NOTE: interrogating option state may be a better way to ascertain that info
    bool option(moducom::coap::Option::Numbers number, uint16_t length = 0)
    {
        option_type option(number);

        option.length = length;

        option_encoder.next(option);

        // TODO: run option encoder up until OptionDeltaAndLengthDone or
        // OptionLengthDone state
        int result;
        do
        {
            result = option_encoder.generate_iterate();
            if(result != -1)
            {
                if(rdbuf()->sputc(result) == traits_type::eof())
                {
                    return false;
                }
            }

        }   while(
                (option_encoder.state() != option_encoder.OptionDeltaAndLengthDone) &&
                (option_encoder.state() != option_encoder.OptionLengthDone));

        return true;
    }


    template <class TImpl>
    bool option(moducom::coap::Option::Numbers number, estd::internal::allocated_array<TImpl>& a)
    {
        if(!option(number, a.size())) return false;

        // FIX: following code is limited.  Firstly, we do it to avoid
        // having to do a lock/unlock on a, but really, is it that big a deal?
        // Secondly, since we're interacting with buffers directly, no sync/underflow
        // and friends get to run, so something like a netbuf-expand never can happen
        streambuf_type* sb = rdbuf();

        int copied = a.copy(sb->pptr(), remaining());

        sb->pubseekoff(copied, estd::ios_base::cur, estd::ios_base::out);

        // FIX: we definitely don't have gaurunteed success here
        // and we don't address chunking yet at all (I don't remember if OptionsEncoder helped us
        // with that... I think it does)
        return true;
    }

    bool option(moducom::coap::Option::Numbers number, const char* str)
    {
        int n = strlen(str);
        option(number, n);
        estd::streamsize written = rdbuf()->sputn(str, n);
        // FIX: we don't address chunking yet at all, even with this (how does it know
        // where in the string to resume from?)
        return written == n;
    }

    // NOTE: Only generates payload marker
    // TODO: Make state machine participate too, maybe with a default flag to turn off if we really really don't
    // want those extra few bytes generated
    bool payload()
    {
        return rdbuf()->sputc(COAP_PAYLOAD_MARKER) != traits_type::eof();
    }

    this_type& operator<<(this_type& (*__pf)(this_type&))
    {
        return __pf(*this);
    }
};

template <class TStreambuf>
StreambufEncoder<TStreambuf>& operator<<(StreambufEncoder<TStreambuf>& encoder, moducom::coap::Header header)
{
    encoder.header(header);

    return encoder;
}

// manipulators

// NOTE: almost definitely have something laying about in our options area to fill this role
struct _Option
{
    moducom::coap::Option::Numbers number;
    uint16_t sz;
};

_Option option(moducom::coap::Option::Numbers number, uint16_t sz)
{
    return _Option { number, sz };
}


struct _Token
{
    estd::const_buffer raw;
};

_Token token(estd::const_buffer raw)
{
    return _Token { raw };
}


template <class TStreambuf>
StreambufEncoder<TStreambuf>& operator<<(StreambufEncoder<TStreambuf>& encoder, _Option o)
{
    encoder.option(o.number, o.sz);

    return encoder;
}

template <class TStreambuf>
StreambufEncoder<TStreambuf>& operator<<(StreambufEncoder<TStreambuf>& encoder, _Token o)
{
    encoder.token(o.raw);

    return encoder;
}


template <class TStreambuf>
inline StreambufEncoder<TStreambuf>& payload(StreambufEncoder<TStreambuf>& encoder)
{
    encoder.payload();

    return encoder;
}

}}}
