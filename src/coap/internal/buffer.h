#pragma once

#include <estd/span.h>

namespace embr { namespace coap {

namespace internal {

// DEBT: Eventually we want this to be estd::byte, not uint8_t
typedef estd::span<const uint8_t> const_buffer;

}

}}