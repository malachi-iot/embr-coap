#include "context.h"

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>


using namespace embr::coap;

bool AppContext::on_notify(const event::option& e)
{
    switch(e.option_number)
    {
        case Option::UriPath:
            switch(found_node())
            {
                case id_path_v1_api_gpio_value:
                    state.emplace<states::gpio>(*this);
                    break;

                case id_path_v1_api_pwm:
                    state.emplace<states::ledc_timer>(*this);
                    break;

                case id_path_v1_api_pwm_value:
                    state.emplace<states::ledc_channel>(*this);
                    break;
            }
            break;

        case Option::UriQuery:
        {
            const query q = split(e);
            const estd::string_view key = estd::get<0>(q);
            
            if(key.data() == nullptr)
            {
                // NOTE: In our case we require key=value format.  But I believe URIs
                // in general are more free form.  So, this IS a malformed query, but
                // only in the context of this application - not a CoAP URI
                ESP_LOGI(TAG, "malformed query: %.*s", e.chunk.size(), e.string().data());
                return false;
            }

            switch(found_node())
            {
                case id_path_v1_api_gpio_value:
                    estd::get<states::gpio>(state).on_option(q);
                    break;

                case id_path_v1_api_pwm:
                    estd::get<states::ledc_timer>(state).on_option(q);
                    break;

                case id_path_v1_api_pwm_value:
                    estd::get<states::ledc_channel>(state).on_option(q);
                    break;
            }
            break;
        }

        default: return false;
    }

    return true;
}


bool AppContext::on_notify(event::completed, encoder_type& encoder)
{
    state.visit_index([&](auto i)
    {
        // FIX: Mostly works, but always is a const
        //return i->completed(encoder);
        return true;
    });

    switch(found_node())
    {
        case id_path_v1_api_analog:
            completed_analog(encoder);
            break;
            
        case id_path_v1_api_gpio_value:
            completed_gpio(encoder);
            break;

        case id_path_v1_api_pwm:
            estd::get<states::ledc_timer>(state).completed(encoder);
            break;

        case id_path_v1_api_pwm_value:
            completed_ledc_channel(encoder);
            estd::get<states::ledc_channel>(state).completed(encoder);
            break;

        default:    return false;
    }

    return true;
}
