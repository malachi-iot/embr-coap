#pragma once

#include <estd/optional.h>

#include <coap/internal/rfc7641/notifier.hpp>
#include <coap/platform/lwip/rfc7641/notifier.hpp>

typedef embr::coap::internal::observable::sequence_type sequence_type;

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

typedef embr::coap::internal::observable::layer1::Registrar<endpoint_type, 10> registrar_type;

typedef embr::coap::internal::observable::Notifier<
    embr::lwip::udp::Pcb,
    registrar_type>
    Notifier;

typedef Notifier::notifier_type::encoder_type encoder_type;

// NOTE: Only builds option and payload portion
void build_stat(encoder_type& encoder, sequence_type sequence = sequence_type());


extern Notifier* notifier;