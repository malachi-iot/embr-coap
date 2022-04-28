#pragma once

#include <estd/span.h>
#include <estd/string_view.h>
#include "../coap.h"

#include "../coap/decoder/events.h"

namespace embr { namespace coap {

struct ExperimentalDecoderEventTypedefs
{
    typedef event::token token_event;
    typedef event::payload payload_event;
    typedef event::option option_event;
    typedef event::option_start option_start_event;
    typedef event::option_completed option_completed_event;
    typedef event::header header_event;
    typedef event::completed completed_event;
};


}}
