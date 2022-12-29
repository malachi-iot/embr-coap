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

template <class TTransport, class TRegistrar>
struct NotifyHelper;

template <class TRegistrar>
struct NotifyHelper<embr::lwip::udp::Pcb, TRegistrar>
{
    embr::lwip::udp::Pcb pcb;

    typedef embr::coap::experimental::observable::lwip::Notifier notifier_type;
    typedef TRegistrar registrar_type;

    registrar_type& registrar;

    NotifyHelper(embr::lwip::udp::Pcb pcb, registrar_type& registrar) :
        pcb(pcb),
        registrar(registrar)
    {
    }

    template <typename F>
    void notify(typename registrar_type::handle_type handle, F&& f)
    {
        notifier_type::notify(registrar, handle, pcb, std::move(f));
    }
};

}

typedef internal::NotifyHelper<
    embr::lwip::udp::Pcb,
    embr::coap::experimental::observable::layer1::Registrar<endpoint_type> >
    NotifyHelper;

typedef NotifyHelper::notifier_type::encoder_type encoder_type;
typedef NotifyHelper::registrar_type registrar_type;

void build_stat(encoder_type& encoder, sequence_type sequence = sequence_type());


extern NotifyHelper* notifier2;