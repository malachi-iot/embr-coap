#pragma once

#include <estd/optional.h>

#include <exp/lwip/subject.hpp>

typedef embr::coap::experimental::observable::lwip::Notifier notifier_type;
typedef embr::coap::experimental::observable::sequence_type sequence_type;
typedef notifier_type::encoder_type encoder_type;

typedef embr::lwip::internal::Endpoint<false> endpoint_type;
typedef embr::coap::experimental::observable::Registrar<endpoint_type> registrar_type;

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

void build_stat(encoder_type& encoder, sequence_type sequence = sequence_type());


struct NotifyHelper
{
    embr::lwip::udp::Pcb pcb;

    registrar_type& registrar;

    NotifyHelper(embr::lwip::udp::Pcb pcb, registrar_type& registrar) :
        pcb(pcb),
        registrar(registrar)
    {
    }

    template <typename F>
    void notify(registrar_type::handle_type handle, F&& f)
    {
        notifier_type::notify(registrar, handle, pcb, std::move(f));
    }
};


extern NotifyHelper* notifier2;