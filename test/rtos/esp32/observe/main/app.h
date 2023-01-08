#pragma once

#include <coap/platform/lwip/rfc7641/notifier.hpp>


namespace paths {

enum
{
    v1 = 0,
    v1_stats,
    v1_load,
    v1_save
};

}

// DEBT: It's about time we liberated 'Endpoint' from internal namespace
typedef embr::lwip::internal::Endpoint<false> endpoint_type;

typedef embr::coap::internal::observable::sequence_type sequence_type;
typedef embr::coap::internal::observable::layer1::Registrar<endpoint_type, 10> registrar_type;

typedef embr::coap::internal::observable::Notifier<
    embr::lwip::udp::Pcb,
    registrar_type>
    Notifier;

typedef Notifier::encoder_type encoder_type;

// NOTE: 'suffix' only builds option and payload portion
void build_stat_suffix(encoder_type& encoder, sequence_type sequence = sequence_type());

embr::coap::Header::Code nvs_load_registrar();
void nvs_save_registrar();
