#pragma once

#include "fwd.h"

namespace embr { namespace coap {

namespace internal {

// For RFC 7641
// TODO: Once promoted from experimental, move this elsewhere
namespace observable {

// Designed to fit into a 2 bit value
enum Options
{
    Register = 0,
    Deregister = 1,

    Sequence = 2,   // EXPERIMENTAL

    Unspecified = 3,
};

// DEBT: Doesn't fit naming convention, and kinda sloppy in general
typedef estd::layer1::optional<Options, Unspecified> option_value_type;

}

}

}}