#pragma once

#include "enum.h"
#include "keys.h"

namespace embr { namespace coap {

namespace internal {

// DEBT: In fact, NotifyHelperBsae is truly the registrar

// DEBT: Consider removing 'Base' and instead specializing on void transport -
// note for that to be aesthetic (which is the whole point), we'd need to swap
// TTransport and TRegistrar order
template <class TRegistrar>
struct NotifyHelperBase
{
    typedef typename estd::remove_reference<TRegistrar>::type registrar_type;
    typedef typename registrar_type::endpoint_type endpoint_type;
    typedef typename registrar_type::handle_type handle_type;

protected:
    TRegistrar registrar;

public:
    ESTD_CPP_CONSTEXPR_RET NotifyHelperBase(const registrar_type& registrar) :
        registrar(registrar)
    {}

    ESTD_CPP_DEFAULT_CTOR(NotifyHelperBase)

    void add(const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
#if __cpp_variadic_templates
        registrar.observers.emplace_back(endpoint, token, handle);
#else
        embr::coap::experimental::observable::RegistrarKey<endpoint_type> key(
            endpoint, token, handle);

        registrar.observers.push_back(key);
#endif
    }


    void remove(const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
        auto i = registrar.observers.begin();
        
        for(;i != registrar.observers.end(); ++i)
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
                if(registrar.full())
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

    unsigned observer_count() const { return registrar.observers.size(); }
};

}

}}
