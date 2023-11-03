#pragma once

#include <driver/ledc.h>

#include "ledc.h"

namespace embr { namespace coap { namespace esp_idf {

inline namespace subcontext { inline namespace v1 {

template <ESTD_CPP_CONCEPT(concepts::IncomingContext) Context, int id_path_>
ledc_timer<Context, id_path_>::ledc_timer(Context& context, const ledc_timer_config_t& def) :
    base_type(context),
    config(def)
{
}

}}

}}}
