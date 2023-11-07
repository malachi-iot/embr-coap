// DEBT: Just for default ledc channel config
#include <driver/ledc.h>

#include "context.h"

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>

#include <coap/platform/esp-idf/subcontext/gpio.hpp>
#include <coap/platform/esp-idf/subcontext/ledc.hpp>


using namespace embr::coap;

// DEBT: Copy/paste from ledc code plus we'd rather not have this much
// ledc specifics in context area
static constexpr ledc_channel_config_t ledc_channel_default = {
    .gpio_num       = 0,
    .speed_mode     = LEDC_LS_MODE,
    .channel        = LEDC_CHANNEL_0,
    .intr_type      = LEDC_INTR_DISABLE,
    .timer_sel      = LEDC_LS_TIMER,
    .duty           = 0, // Set duty to 0%
    .hpoint         = 0,
    .flags {
        .output_invert = 0
    }
};


bool AppContext::on_notify(const event::option& e)
{
    switch(e.option_number)
    {
        case Option::UriPath:
        {
            const int node = found_node();

            state.create(node, *this, [&]<class T>(estd::in_place_type_t<T>)
            {
                if constexpr(estd::is_same_v<T, states::gpio>)
                {
                    return e;
                }
                else if constexpr(estd::is_same_v<T, states::ledc_timer>)
                {
                    return ledc_timer_config_t {};
                }
                else if constexpr(estd::is_same_v<T, states::ledc_channel>)
                {
                    return estd::make_tuple(ledc_channel_default, e);
                }
                else
                    return nullptr_t{};
            });

            /*
            // DEBT: Not available, think we'd like to expose ::types - though
            // really we do want visitor
            //using state_types = decltype(state)::types;
            //using state_visitor = decltype(state)::visitor;
            // DEBT: 'visit' naming here a little dubious, because we usually have no business visiting
            // all the instances in a variant.  This is an exceptional case.  Perhaps we should rename
            // it to 'visit_instance' to reflect that? - just as variant_storage does
            // We aren't actually using the instance here anyway
            state.visit([this, node]<unsigned I, Subtate T>(estd::variadic::v2::instance<I, T> vi)
            {
                // All that above DEBT aside, we have a badass emplace factory!
                if(node != T::id_path) return false;

                state.emplace<T>(*this);
                return true;
            });
            */
            break;
        }

        case Option::UriQuery:
            state.on_uri_query(e, *this);
            break;

        default: return false;
    }

    return true;
}
