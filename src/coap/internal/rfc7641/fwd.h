#pragma once

#include <estd/optional.h>

namespace embr { namespace coap {

namespace internal {

// DEBT: Consider removing 'Base' and instead specializing on void transport -
// note for that to be aesthetic (which is the whole point), we'd need to swap
// TTransport and TRegistrar order
template <class TRegistrar>
struct NotifyHelperBase;

namespace observable {

// DEBT: Doesn't fit naming convention, and kinda sloppy in general
typedef estd::layer1::optional<uint32_t, 0x1000000> sequence_type;

namespace detail {

template <class TContainer>
struct Registrar;

}

}

}

}}