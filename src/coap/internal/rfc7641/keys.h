#pragma once

#include <estd/internal/platform.h>
#include <estd/variant.h>

#include "fwd.h"

namespace embr { namespace coap {

namespace internal {

template <class TEndpoint>
struct EndpointProvider
{
    typedef TEndpoint endpoint_type;

    const endpoint_type endpoint;

    ESTD_CPP_CONSTEXPR_RET EndpointProvider(endpoint_type endpoint) :
        endpoint(endpoint)
    {

    }
};

// Uniquely identifies a message
// NOTE: TTimePoint may or may not participate in uniquely identifying the message, depending
// on the task at hand
// initially for use with dup mid matcher, but could be useful for observable as well
template <typename TEndpoint, typename TTimePoint = void>
struct MessageKey : EndpointProvider<TEndpoint>
{
    typedef TTimePoint timepoint;

    const uint16_t mid;
    const timepoint timestamp;
};

template <typename TEndpoint>
struct MessageKey<TEndpoint, void> : EndpointProvider<TEndpoint>
{
    typedef EndpointProvider<TEndpoint> base_type;

    const uint16_t mid;

    ESTD_CPP_CONSTEXPR_RET MessageKey(TEndpoint endpoint, uint16_t mid) :
        base_type(endpoint),
        mid(mid)
    {}
};

// For use with RFC 7641
template <typename TEndpoint,
    embr::coap::internal::observable::detail::SequenceTracking = observable::detail::SequenceTracking::PerObserver>
struct ObserveEndpointKey : EndpointProvider<TEndpoint>
{
    typedef EndpointProvider<TEndpoint> base_type;
    typedef typename base_type::endpoint_type endpoint_type;
    typedef coap::layer2::Token token_type;

    const token_type token;

    // NOTE: Not used at this time
    // DEBT: Filter this out by with_sequence to optimize storage
    // DEBT: Use uint of only 3 bytes as per RFC 7641 Section 3.4
    observable::sequence_type sequence;

    ESTD_CPP_CONSTEXPR_RET ObserveEndpointKey(endpoint_type endpoint,
        const estd::span<const uint8_t>& token) :
        base_type(endpoint),
        token(token)
    {}

    ESTD_CPP_CONSTEXPR_RET ObserveEndpointKey(endpoint_type endpoint, const token_type& token) :
        base_type(endpoint),
        token(token)
    {}

    ESTD_CPP_CONSTEXPR_RET ObserveEndpointKey(const ObserveEndpointKey& copy_from) :
        base_type(copy_from.endpoint),
        token(copy_from.token)
    {}
};

namespace observable {

struct RegistrarKeyBase
{
    typedef int handle_type;
    // this represents the particular resource being observed.
    // For simple scenarios, there will be just 1
    const handle_type handle;

    union
    {
        void*       ptr;
        uintptr_t   data;
    } arg;

    RegistrarKeyBase(handle_type handle) : handle(handle) {}
};

template <class TEndpoint, class TAppData = estd::monostate>
struct RegistrarKey : ObserveEndpointKey<TEndpoint>,
    RegistrarKeyBase,
    TAppData
{
    typedef TEndpoint endpoint_type;
    typedef ObserveEndpointKey<endpoint_type> base_type;

    ESTD_CPP_CONSTEXPR_RET RegistrarKey(const endpoint_type& endpoint,
        const estd::span<const uint8_t>& token,
        handle_type handle) :
        base_type(endpoint, token),
        RegistrarKeyBase(handle)
    {}

    ESTD_CPP_CONSTEXPR_RET RegistrarKey(const base_type& observer, handle_type handle) :
        base_type(observer),
        RegistrarKeyBase(handle)
    {}

    ESTD_CPP_CONSTEXPR_RET RegistrarKey(const RegistrarKey& copy_from) :
        base_type(copy_from),
        RegistrarKeyBase(copy_from.handle)
    {}

    RegistrarKey& operator =(const RegistrarKey& copy_from)
    {
        return * new (this) RegistrarKey(copy_from);
    }
};

}

}

}}