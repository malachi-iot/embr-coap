/**
 * @file
 */
#pragma once

#include "../../coap-features.h"

#if FEATURE_MCCOAP_NETBUF_ENCODER
#include <embr/streambuf.h>
#endif
#include <estd/streambuf.h>
#include <estd/ostream.h>

#include "../header.h"
#include "../../coap-encoder.h"
#include "../../coap_transmission.h"
#include "../context.h"
#include "../uint.h"
#include "factory.h"

namespace moducom { namespace coap {

namespace impl {

template<class TStreambuf>
struct StreambufEncoderImpl
{
    typedef typename estd::remove_const<TStreambuf>::type streambuf_type;

    static void finalize(streambuf_type* streambuf) {}
};

#if FEATURE_MCCOAP_NETBUF_ENCODER
template <class TNetbuf>
struct StreambufEncoderImpl<::embr::mem::out_netbuf_streambuf<char, TNetbuf> >
{
    typedef typename estd::remove_const<::embr::mem::out_netbuf_streambuf<char, TNetbuf>>::type streambuf_type;
    typedef typename streambuf_type::size_type size_type;

    static void finalize(streambuf_type* streambuf)
    {
        size_type total_length = streambuf->absolute_pos();

        streambuf->netbuf().shrink(total_length);
    }
};
#endif

}


// experimental replacement for netbuf encoder.  it's possible streambufs
// are an all around replacement for netbufs, or at least most of the time
// TODO: Still need to address partial (chunked) writes
// NOTE: header and token no chunking is not planned, we'd have to employ HeaderEncoder and
// TokenEncoder.  That use case is an edge case, since maximum size of header + token = 12 bytes
template <
        class TStreambuf,
        class TStreambufEncoderImpl = impl::StreambufEncoderImpl<TStreambuf>
        >
class StreambufEncoder
{
#ifdef FEATURE_CPP_ENUM_CLASS
    typedef moducom::coap::internal::Root::State state_type;
    // DEBT: For passing around as parameter, needs better name
    typedef state_type _state_type;
#else
    typedef moducom::coap::internal::Root state_type;
    // DEBT: For passing around as parameter, needs better name
    typedef moducom::coap::internal::Root::State _state_type;
#endif

#ifdef DEBUG
    void _assert(_state_type s)
    {
        // TODO:
    }
#else
    void _assert(_state_type s) {}
    /// \brief Only used for debug purposes.  noop in release mode
    /// \param s
    void state(_state_type s) {}
#endif

    typedef StreambufEncoder<TStreambuf> this_type;

public:
    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;
    typedef TStreambufEncoderImpl streambuf_encoder_traits;
    typedef estd::internal::basic_ostream<streambuf_type&> ostream_type;
    typedef moducom::coap::experimental::layer2::OptionBase option_type;
    typedef moducom::coap::Option::Numbers option_number_type;

protected:
    // DEBT: Must assert that char_type is an 8-bit type
    typedef typename streambuf_type::traits_type traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    estd::streamsize write(const uint8_t* bytes, estd::streamsize n)
    {
        return rdbuf()->sputn(reinterpret_cast<const char_type*>(bytes), n);
    }

    // returns true when character written successfully, false otherwise
    bool write(char_type c)
    {
        int_type result = rdbuf()->sputc(c);
        //return traits_type::not_eof(result) == traits_type::to_int_type(c);
        return !traits_type::eq_int_type(result, traits_type::eof());
        return result != traits_type::eof();
    }

protected:

    TStreambuf streambuf;
    moducom::coap::OptionEncoder option_encoder;

    /*
    estd::streamsize remaining()
    {
        return rdbuf()->epptr() - rdbuf()->pptr();
    } */

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
#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    StreambufEncoder(TArgs&&... args) :
        streambuf(std::forward<TArgs>(args)...) {}
#endif

#ifdef FEATURE_CPP_MOVESEMANTIC
    StreambufEncoder(streambuf_type&& streambuf) :
        streambuf(std::move(streambuf)) {}
#endif

#ifndef FEATURE_CPP_VARIADIC
    // FIX: Replace this with the good-ol TParam1 C++03 compat way of doin gthings,
    // hardcoding span in here is just temporary
    StreambufEncoder(const estd::span<char_type>& span) : streambuf(span) {}
#endif

    // ostream-style
    streambuf_type* rdbuf() { return &streambuf; }
    const streambuf_type* rdbuf() const { return &streambuf; }

    // NOTE: might prefer instead to present entire encoder as a minimal
    // (non flaggable) basic_ostream.  Brings along some useful functionality
    // without adding any overhead
    ostream_type ostream() { return ostream_type(streambuf); }

    void header(const moducom::coap::Header& header)
    {
        _assert(state_type::Uninitialized);
        // TODO: Chunked not handled, but we could still check for fail to write
        write(header.bytes, 4);
        state(state_type::HeaderDone);
    }


    // NOTE: token size must have already been specified in header!
    void token(const estd::const_buffer token)
    {
        _assert(state_type::HeaderDone);
        // TODO: Chunked not handled, but we could still check for fail to write
        write(token.data(), token.size());
        state(state_type::TokenDone);
    }


    void token(const uint8_t* token, int tkl)
    {
        _assert(state_type::HeaderDone);
        write(token, tkl);
        state(state_type::TokenDone);
    }


    template <bool inline_token, bool simple_buffer>
    void header_and_token(const moducom::coap::TokenAndHeaderContext<inline_token, simple_buffer>&
    ctx)
    {
        header(ctx.header());
        token(ctx.token());
    }

    // FIX: rename to 'option'
    bool option_int(option_number_type number, int option_value)
    {
        moducom::coap::layer2::UInt<> v;

        v.set(option_value);

        return option(number, v);
    }

    // returns false if chunking is needed
    // returns true if entire data was output
    // NOTE: interrogating option state may be a better way to ascertain that info
    // FIX: rename to 'option_raw'
    bool option(moducom::coap::Option::Numbers number, uint16_t length = 0)
    {
        option_type option(number);

        option.length = length;

        option_encoder.next(option);

        // TODO: run option encoder up until OptionDeltaAndLengthDone or
        // OptionLengthDone state
        int_type result;
        do
        {
            result = option_encoder.generate_iterate();
            if(result != -1)
            {
                if(!write(result))
                    return false;
            }

        // FIX: Appears to have a glitch where we need to proceed one past this so that delta tracker works
        }   while(
                //(option_encoder.state() != option_encoder.OptionDeltaAndLengthDone) &&
                (option_encoder.state() != option_encoder.OptionLengthDone));

        return true;
    }


    template <class TImpl>
    bool option(moducom::coap::Option::Numbers number, estd::internal::allocated_array<TImpl>& a)
    {
        typedef typename estd::internal::allocated_array<TImpl>::value_type value_type;

        // TODO: ensure a.size() is size in bytes
        if(!option(number, a.size())) return false;

        // TODO: ensure value_type is 8-bit (strong overlap with
        // checking that size is in bytes)
        const value_type* a_data = a.clock();
        const char_type* s = reinterpret_cast<const char_type*>(a_data);
        rdbuf()->sputn(s, a.size());
        a.cunlock();

        /*

        // FIX: following code is limited.  Firstly, we do it to avoid
        // having to do a lock/unlock on a, but really, is it that big a deal?
        // Secondly, since we're interacting with buffers directly, no sync/underflow
        // and friends get to run, so something like a netbuf-expand never can happen
        streambuf_type* sb = rdbuf();

        int copied = a.copy(sb->pptr(), remaining());

        sb->pubseekoff(copied, estd::ios_base::cur, estd::ios_base::out);

         */

        // FIX: we definitely don't have gaurunteed success here
        // and we don't fully address chunking yet (I don't remember if OptionsEncoder helped us
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
        return write(COAP_PAYLOAD_MARKER);
    }

    // since CoAP depends on transport packet size, utilize streambuf_encoder_traits
    // to demarkate/process output streambuf in the specific way needed for coap
    // encoding
    // For UDP, this means ensuring the UDP packet is exactly the size of the CoAP packet, which
    // for LwIP, usually means realloc (shrinking) the PBUF chain
    void finalize()
    {
        streambuf_encoder_traits::finalize(rdbuf());
    }

    this_type& operator<<(this_type& (*__pf)(this_type&))
    {
        return __pf(*this);
    }
};

namespace experimental {

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

inline _Option option(moducom::coap::Option::Numbers number, uint16_t sz)
{
#ifdef __cpp_initializer_lists
    return _Option { number, sz };
#else
    _Option option;
    option.number = number;
    option.sz = sz;
    return option;
#endif
}


struct _Token
{
    estd::const_buffer raw;

#ifndef __cpp_initializer_lists
    _Token(estd::const_buffer raw) : raw(raw) {}
#endif
};

inline _Token token(estd::const_buffer raw)
{
#ifdef __cpp_initializer_lists
    return _Token { raw };
#else
    return _Token(raw);
#endif
}


template <class TStreambuf>
inline StreambufEncoder<TStreambuf>& operator<<(StreambufEncoder<TStreambuf>& encoder, _Option o)
{
    encoder.option(o.number, o.sz);

    return encoder;
}

template <class TStreambuf>
inline StreambufEncoder<TStreambuf>& operator<<(StreambufEncoder<TStreambuf>& encoder, _Token o)
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
