#pragma once

#include <estd/span.h>
#include <estd/string_view.h>
#include "../coap.h"

namespace moducom { namespace coap {

namespace experimental {

// 6/24/2018 revamped event system based on upcoming estd::experimental::subject code

// using only for typedef convenience
struct event_base
{
    typedef estd::const_buffer buffer_t;
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

struct option_event : chunk_event_base
{
    option_number_t option_number;

    option_event(uint16_t n) :
            option_number((option_number_t)n) {}

    option_event(uint16_t n,
                 const buffer_t& chunk,
                bool last_chunk) :
            chunk_event_base(chunk, last_chunk),
            option_number((option_number_t)n)
    {}
};

struct option_start_event {};

// FIX: naming inconsistency with Decoder::OptionsDone
struct option_completed_event {};

// doing struct instead of typedef to ensure it overloads as
// a different type during on_notify
struct payload_event : chunk_event_base
{
    payload_event(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};


template <class TStreambuf>
struct streambuf_payload_event : streambuf_event_base<TStreambuf>
{
    typedef streambuf_event_base<TStreambuf> base_type;
    typedef typename base_type::streambuf_type streambuf_type;

    streambuf_payload_event(streambuf_type& streambuf) : base_type(streambuf) {}
};


struct completed_event {};


struct header_event
{
    Header header;

    header_event(const Header& header) :
        header(header) {}
};


struct token_event : chunk_event_base
{
    token_event(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};


// this event applies to a different set of code, the revised version of uripath-dispatcher
struct uri_path_event
{
    estd::string_view path;
};

}

}}
