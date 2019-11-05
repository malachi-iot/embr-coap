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

namespace embr { namespace coap { namespace experimental {

// experimental replacement for netbuf encoder.  it's possible streambufs
// are an all around replacement for netbufs, or at least most of the time
// TODO: Still need to address partial (chunked) writes
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

public:
    typedef TStreambuf streambuf_type;
    typedef estd::internal::basic_ostream<streambuf_type&> ostream_type;
    typedef moducom::coap::experimental::layer2::OptionBase option_type;

protected:
    streambuf_type streambuf;
    moducom::coap::OptionEncoder option_encoder;

    estd::streamsize sputn(const uint8_t* bytes, estd::streamsize n)
    {
        return rdbuf()->sputn(reinterpret_cast<const char*>(bytes), n);
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
        sputn(header.bytes, 4);
        state(state_type::HeaderDone);
    }


    // NOTE: token size must have already been specified in header!
    void token(const estd::const_buffer token)
    {
        sputn(token.data(), token.size());
    }


    template <bool inline_token, bool simple_buffer>
    void header_and_token(const moducom::coap::TokenAndHeaderContext<inline_token, simple_buffer>&
    ctx)
    {
        header(ctx.header());
        token(ctx.token());
    }

    void option(moducom::coap::Option::Numbers number, uint16_t length = 0)
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
            if(result != -1) rdbuf()->sputc(result);

        }   while(
                (option_encoder.state() != option_encoder.OptionDeltaAndLengthDone) &&
                (option_encoder.state() != option_encoder.OptionLengthDone));

    }


    template <class TImpl>
    void option(moducom::coap::Option::Numbers number, estd::internal::allocated_array<TImpl>& a)
    {
        option(number, a.size());

        streambuf_type* sb = rdbuf();

        int copied = a.copy(sb->pptr(), remaining());

        sb->pubseekoff(copied, estd::ios_base::cur, estd::ios_base::out);
    }

    void option(moducom::coap::Option::Numbers number, const char* str)
    {
        int n = strlen(str);
        option(number, n);
        rdbuf()->sputn(str, n);
    }

    // NOTE: Only generates payload marker
    void payload()
    {
        rdbuf()->sputc(0xFF);
    }
};

}}}
