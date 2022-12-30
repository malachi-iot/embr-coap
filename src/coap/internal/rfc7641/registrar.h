#pragma once

#include "keys.h"

namespace embr { namespace coap {


namespace internal { namespace observable {

namespace detail {

struct RegistrarBase
{
    typedef RegistrarKeyBase::handle_type handle_type;
};

template <class TContainer>
struct Registrar : RegistrarBase
{
    typedef TContainer container_type;
    typedef typename container_type::value_type Key;
    //typedef RegistrarKey<endpoint_type> Key;
    typedef typename Key::endpoint_type endpoint_type;
    typedef ObserveEndpointKey<endpoint_type> key_type;

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

    bool full() const
    {
        // DEBT: Make a full() in dynamic_array itself who consults impl().is_allocated() as well
        return observers.size() >= observers.capacity();
    }

    // TODO: Add registrar which takes only observer to remove all instances of that observer,
    // as well as a similar flavor for handle
};

}



namespace layer1 {

template <class TEndpoint, unsigned N>
struct Registrar : detail::Registrar<estd::layer1::vector<RegistrarKey<TEndpoint>, N> >
{
};


}

}}

}}