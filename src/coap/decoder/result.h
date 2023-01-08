#pragma once

#include <estd/cstdint.h>

namespace embr { namespace coap {

namespace iterated {

// DEBT: Temporarily putting this here as we flesh out this upgraded return type
struct decode_result
{
    /// End of data stream was reached.  NOTE that this is not considered
    /// a stopping point, as further state machine processing may need to
    /// occur on gathered data.  Look to 'done'
    uint16_t eof : 1;
    /// Data is in an unavailable/unknown state
    uint16_t waitstate : 1;
    /// CoAP decoding encountered an error - not yet active
    uint16_t failure : 1;
    /// CoAP completed decoding the packet normally
    uint16_t done: 1;

#ifndef __cpp_initializer_lists
    decode_result() {}
    decode_result(bool eof, bool waitstate, bool failure, bool done) :
        eof(eof), waitstate(waitstate), failure(failure), done(done)
    {}
#else
    // As of c++20 this interferes with initializer list
    // https://en.cppreference.com/w/cpp/language/aggregate_initialization
    //decode_result() = default;
#endif
    /*
     * FIX: Something is broken about this
    decode_result(estd::nullptr_t) :
        eof(0), waitstate(0), failure(0), done(0)
    {} */
};

}

}}