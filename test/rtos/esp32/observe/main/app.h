#pragma once

#include <estd/optional.h>

#include <exp/lwip/subject.hpp>

typedef embr::coap::experimental::observable::sequence_type sequence_type;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;

namespace paths {

enum
{
    v1 = 0,
    v1_api,
    v1_api_gpio,
    v1_api_version,
    v1_api_stats,
    v1_api_gpio_value,

    well_known,
    well_known_core
};


}

namespace internal {

// DEBT: Since underlying lwip::Notifier (and similar) are pretty fused with this,
// change internal::NotifyHelper to regular Notifier and change underyling lwip::Notifier
// to impl
template <class TTransport, class TRegistrar>
struct NotifyHelper;

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

    embr::coap::Header::Code::Codes add_or_remove(
        embr::coap::experimental::observable::option_value_type option_value,
        const endpoint_type& endpoint,
        estd::span<const uint8_t> token,
        handle_type handle)
    {
        switch(option_value.value())
        {
            case embr::coap::experimental::observable::Register:
                //notifier->registrar.add(key, path_id);
                add(endpoint, token, handle);
                return embr::coap::Header::Code::Valid;

            case embr::coap::experimental::observable::Deregister:
                // not ready yet
                //registrar.remove(key, path_id);
                return embr::coap::Header::Code::NotImplemented;

            default:
                return embr::coap::Header::Code::InternalServerError;
        }
    }

    unsigned observer_count() const { return registrar.observers.size(); }
};

template <class TRegistrar>
struct NotifyHelper<embr::lwip::udp::Pcb, TRegistrar> :
    NotifyHelperBase<TRegistrar>
{
    typedef NotifyHelperBase<TRegistrar> base_type;
    typedef typename base_type::registrar_type registrar_type;

    embr::lwip::udp::Pcb pcb;

    // DEBT: This is shaping up to be an impl
    typedef embr::coap::experimental::observable::lwip::Notifier notifier_type;

    NotifyHelper(embr::lwip::udp::Pcb pcb) :
        pcb(pcb)
    {
    }

    NotifyHelper(embr::lwip::udp::Pcb pcb, registrar_type& registrar) :
        pcb(pcb),
        base_type(registrar)
    {
    }

    template <typename F>
    void notify(typename registrar_type::handle_type handle, F&& f)
    {
        notifier_type::notify(base_type::registrar, handle, pcb, std::move(f));
    }
};

}

typedef internal::NotifyHelper<
    embr::lwip::udp::Pcb,
    embr::coap::experimental::observable::layer1::Registrar<endpoint_type> >
    NotifyHelper;

typedef NotifyHelper::notifier_type::encoder_type encoder_type;
typedef NotifyHelper::registrar_type registrar_type;

// NOTE: Only builds option and payload portion
void build_stat(encoder_type& encoder, sequence_type sequence = sequence_type());


extern NotifyHelper* notifier;