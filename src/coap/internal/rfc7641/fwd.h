#pragma once

#include <estd/optional.h>

namespace embr { namespace coap {

namespace internal {

template <class TRegistrar>
struct NotifyHelperBase;

namespace observable {

template <class TTransport, class TRegistrar>
struct Notifier;

// DEBT: Doesn't fit naming convention, and kinda sloppy in general
typedef estd::layer1::optional<uint32_t, 0x1000000> sequence_type;

namespace detail {

template <class TContainer>
struct Registrar;

}

}

}

}}