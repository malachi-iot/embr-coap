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

    // DEBT: Make this private/protected
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

    void add(const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
#if __cpp_variadic_templates
        observers.emplace_back(endpoint, token, handle);
#else
        embr::coap::experimental::observable::RegistrarKey<endpoint_type> key(
            endpoint, token, handle);

        observers.push_back(key);
#endif
    }


    void remove(const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
        auto i = observers.begin();

        for(;i != observers.end(); ++i)
        {
            const observable::RegistrarKey<endpoint_type>& key = *i;

            if(key.endpoint == endpoint &&
               key.handle == handle &&
               equal(key.token.span(), token))
                // FIX: it's time to typedef token type as a layerX vector or span
                // directly, though span's lack of == presents a challenge
                //key.token == token)
            {
            }
        }
    }

    Header::Code add_or_remove(
        observable::option_value_type option_value,
        const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
        switch(option_value.value())
        {
            case observable::Register:
                if(full())
                    return Header::Code::ServiceUnavailable;

                add(endpoint, token, handle);
                return Header::Code::Created;

                // UNDEFINED behavior, need to research what is expected
                // specifically on a deregister
            case observable::Deregister:
                remove(endpoint, token, handle);
                return Header::Code::NotImplemented;

            default:
                return Header::Code::InternalServerError;
        }
    }

    unsigned observer_count() const { return observers.size(); }

    // TODO: Add 'remove' which takes only observer to remove all instances of that observer,
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