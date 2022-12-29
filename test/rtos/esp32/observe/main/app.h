#pragma once

#include <estd/optional.h>

#include <exp/lwip/subject.hpp>

typedef embr::coap::experimental::observable::lwip::Notifier notifier_type;
typedef embr::coap::experimental::observable::sequence_type sequence_type;
typedef notifier_type::encoder_type encoder_type;

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