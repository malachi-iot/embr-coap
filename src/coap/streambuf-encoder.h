/**
 * @file
 */
#pragma once

#include <embr/streambuf.h>
#include <estd/streambuf.h>
#include <estd/ostream.h>

#include "../coap-header.h"
#include "../coap-encoder.h"
#include "../coap_transmission.h"

namespace embr { namespace coap { namespace experimental {

// experimental replacement for netbuf encoder.  it's possible streambufs
// are an all around replacement for netbufs, or at least most of the time
// TODO: Still need to address partial (chunked) writes
class StreambufEncoder
{
public:
    typedef estd::internal::streambuf<
            estd::internal::impl::out_span_streambuf<uint8_t> > streambuf_type;
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
        // FIX: eptr is broken, so fake this
        estd::streamsize max_size = 20;
        //size_t max_size = rdbuf()->epptr() - rdbuf()->pptr();
        return max_size;
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
        sputn(header.bytes, 4);
    }


    // NOTE: token size must have already been specified in header!
    void token(const estd::const_buffer token)
    {
        sputn(token.data(), token.size());
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

        int copied = a.copy(rdbuf()->pptr(), remaining());
        // FIX: pbump not yet defined for span output streambuf
        //rdbuf()->pbump(copied);
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
