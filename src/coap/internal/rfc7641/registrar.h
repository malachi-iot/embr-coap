#pragma once

#include "keys.h"

namespace embr { namespace coap {


namespace internal { namespace observable {

template <class TRegistrar>
struct RegistrarTraits;

namespace detail {

struct RegistrarBase
{
    typedef RegistrarKeyBase::handle_type handle_type;
};

template <SequenceTracking>
struct RegistrarSequenceBase {};

template <>
struct RegistrarSequenceBase<SequenceTracking::Singleton>
{
    // Singleton style sequence number tracking
    // DEBT: Track this as a uint for only 3 bytes instead of 4
    // DEBT: Get to the bottom of whether 0 and 1 are permissible sequence numbers, or whether
    // they interfere with coap observe signaling behavior
    uint32_t sequence = 1;
};

template <class TContainer, SequenceTracking sequence_tracking>
struct Registrar : RegistrarBase,
    protected RegistrarSequenceBase<sequence_tracking>
{
    typedef TContainer container_type;

    friend class RegistrarTraits<Registrar<TContainer, sequence_tracking> >;

    // DEBT: I think vector needs this
    //typedef typename container_type::const_referece const_reference;
    typedef typename container_type::value_type value_type;
    typedef typename container_type::iterator iterator;
    typedef const value_type& const_reference;

    typedef typename value_type::endpoint_type endpoint_type;

    // underlying key portion only containing observer identification (endpoint, token, etc)
    typedef ObserveEndpointKey<endpoint_type, SequenceTracking::Singleton> key_type;

    // DEBT: Make this private/protected
    container_type observers;

    void add(key_type observer, handle_type handle)
    {
        // DEBT: Check boundary
        value_type key(observer, handle);

        observers.push_back(key);
    }

    bool remove(const key_type& observer, handle_type handle)
    {
        value_type key(observer, handle);

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
            const const_reference key = *i;

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

template <class TEndpoint, unsigned N, detail::SequenceTracking sequence_tracking = detail::SequenceTracking::Singleton>
struct Registrar : detail::Registrar<estd::layer1::vector<RegistrarKey<TEndpoint, sequence_tracking>, N>, sequence_tracking>
{
};


}


template <class TContainer>
struct RegistrarTraits<detail::Registrar<TContainer, detail::SequenceTracking::Singleton> >
{
    typedef detail::Registrar<TContainer, detail::SequenceTracking::Singleton> registrar_type;
    typedef registrar_type::key_type key_type;
    // DEBT: A mild confusion when used alongside sequence_type - when retrieving sequence numbers,
    // we never expect a "null", so we use an intrinsic here
    typedef uint32_t sq_type;

    typedef estd::integral_constant<detail::SequenceTracking, detail::SequenceTracking::Singleton>
        sequence_tracking_type;

    static ESTD_CPP_CONSTEXPR_RET detail::SequenceTracking sequence_tracking()
    {
        return detail::SequenceTracking::Singleton;
    }

    static sq_type sequence(const registrar_type& r, const key_type&)
    {
        return r.sequence;
    }

    static sq_type increment_sequence(registrar_type& r, key_type&)
    {
        return ++r.sequence;
    }

    // NOTE: The following API is only available for Singleton style, so use with care
    static sq_type sequence(const registrar_type& r)
    {
        return r.sequence;
    }

    static sq_type increment_sequence(registrar_type& r)
    {
        return ++r.sequence;
    }
};


template <class TEndpoint, unsigned N, detail::SequenceTracking st>
struct RegistrarTraits<layer1::Registrar<TEndpoint, N, st> > :
    RegistrarTraits<
        detail::Registrar<typename layer1::Registrar<TEndpoint, N, st>::container_type, st>
    >
{

};



}}

}}