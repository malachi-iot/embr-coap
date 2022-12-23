#pragma once

#include <estd/algorithm.h>
#include <estd/vector.h>

#include "../coap/decoder/observer/util.h"

// RFC 7641 subject - different than embr subject, though same abstract pattern
// Consider this "v2" of subject/observer pattern

namespace embr { namespace coap {

namespace experimental { namespace observable {

template <class TEndpoint>
struct Registrar
{
    typedef TEndpoint endpoint_type;
    typedef ObserveEndpointKey<endpoint_type> key_type;

    struct Key : ObserveEndpointKey<endpoint_type>
    {
        typedef ObserveEndpointKey<endpoint_type> base_type;

        // this represents the particular resource being observed.
        // For simple scenarios, there will be just 1
        const int handle;

        ESTD_CPP_CONSTEXPR_RET Key() :
            // DEBT: undefined token a nasty idea
            base_type(endpoint_type(), coap::layer2::Token()),
            handle(-1)
        {}

        ESTD_CPP_CONSTEXPR_RET Key(const key_type& observer, int handle) :
            base_type(observer),
            handle(handle)
        {}

        /*
        Key(const Key& copy_from) :
            base_type(copy_from),
            handle(copy_from.handle)
        {}  */

        Key& operator =(const Key& copy_from)
        {
            return * new (this) Key(copy_from, copy_from.handle);
        }
    };

    typedef estd::layer1::vector<Key, 10> container_type;

    container_type observers;

    void add(key_type observer, int handle)
    {
        // DEBT: Check boundary
        Key key(observer, handle);

        observers.push_back(key);
    }

    bool remove(const key_type& observer, int handle)
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