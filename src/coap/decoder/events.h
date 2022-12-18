#pragma once

#include <estd/span.h>
#include <estd/string.h>
#include <estd/string_view.h>

// FIX: string_view needs include guards
//#include <estd/string_view.h>
#include "../option.h"
#include "../header.h"

namespace embr { namespace coap { namespace event {

// 6/24/2018 revamped event system based on upcoming estd::experimental::subject code
// 12/3/2019 code works very well, prefers embr's subject/observer.  Proper naming
//           is all that remains to get us out of this experimental namespace.

// using only for typedef convenience
struct event_base
{
    typedef estd::span<const uint8_t> buffer_t;
    typedef internal::option_number_t option_number_t;
};

template <class TStreambuf>
struct streambuf_event_base
{
    typedef typename estd::remove_reference<TStreambuf>::type streambuf_type;

    streambuf_type& streambuf;

    streambuf_event_base(streambuf_type& streambuf) : streambuf(streambuf) {}
};

struct chunk_event_base : event_base
{
    buffer_t chunk;
    bool last_chunk;

    chunk_event_base() :
            chunk(NULLPTR, 0),
            last_chunk(true) {}

    chunk_event_base(const buffer_t& chunk, bool last_chunk) :
            chunk(chunk),
            last_chunk(last_chunk) {}

    estd::layer3::const_string string() const
    {
        return chunk;
    }
};

struct option : chunk_event_base
{
    option_number_t option_number;

    option(uint16_t n) :
            option_number((option_number_t)n) {}

    option(uint16_t n,
           const buffer_t& chunk,
           bool last_chunk) :
            chunk_event_base(chunk, last_chunk),
            option_number((option_number_t)n)
    {}
};

struct option_start {};

namespace internal {

struct no_paylod {};

}

// FIX: naming inconsistency with Decoder::OptionsDone
struct option_completed {};

namespace tags {

struct payload {};

}

// doing struct instead of typedef to ensure it overloads as
// a different type during on_notify
struct payload : chunk_event_base,
    tags::payload
{
    payload(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};


template <class TStreambuf>
struct streambuf_payload : streambuf_event_base<TStreambuf>,
    tags::payload
{
    typedef streambuf_event_base<TStreambuf> base_type;
    typedef typename base_type::streambuf_type streambuf_type;

    streambuf_payload(streambuf_type& streambuf) : base_type(streambuf) {}
};


struct completed
{
    const bool payload_present;

    ESTD_CPP_CONSTEXPR_RET completed(bool payload_present) :
        payload_present(payload_present)
    {}
};


struct header
{
    Header h;

    header(const Header& header) :
            h(header) {}
};


struct token : chunk_event_base
{
    token(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};

// this event applies to a different set of code, the revised version of uripath-dispatcher
struct uri_path
{
    estd::string_view path;
};


}}}
