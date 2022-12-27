#pragma once

#include <estd/algorithm.h>
#include <estd/vector.h>

#include "../coap/decoder/observer/util.h"

// RFC 7641 subject - different than embr subject, though same abstract pattern
// Consider this "v2" of subject/observer pattern

namespace embr { namespace coap {

namespace experimental { namespace observable {

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

// DEBT: Keeping this outside of Registrar because we need to specialize
// on lwip endpoint version to feed in default values for vector to be satisfied.
// All this culminates in I need to really add copy/move assignable ONLY behavior
// to vector (std::vector does not use default constructor for T)
template <class TEndpoint>
struct RegistrarKey : ObserveEndpointKey<TEndpoint>,
    RegistrarKeyBase
{
    typedef TEndpoint endpoint_type;
    typedef ObserveEndpointKey<endpoint_type> base_type;

    ESTD_CPP_CONSTEXPR_RET RegistrarKey() :
        // DEBT: undefined token a nasty idea
        base_type(endpoint_type(), coap::layer2::Token()),
        RegistrarKeyBase(-1)
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
        return * new (this) RegistrarKey(copy_from, copy_from.handle);
    }
};

template <class TEndpoint>
struct Registrar
{
    typedef TEndpoint endpoint_type;
    typedef ObserveEndpointKey<endpoint_type> key_type;
    typedef RegistrarKeyBase::handle_type handle_type;
    typedef RegistrarKey<endpoint_type> Key;

    typedef estd::layer1::vector<Key, 10> container_type;

    container_type observers;

    void add(key_type observer, handle_type handle)
    {
        // DEBT: Check boundary
        Key key(observer, handle);

        observers.push_back(key);
    }

    bool remove(const key_type& observer, handle_type handle)
    {
        Key key(observer, handle);

        typename container_type::iterator i = std::find(observers.begin(), observers.end(), key);

        if(i == std::end(observers)) return false;

        observers.erase(i);
        return true;
    }

    // TODO: Add registrar which takes only observer to remove all instances of that observer,
    // as well as a similar flavor for handle
};

}}

}}