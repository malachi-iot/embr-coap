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
