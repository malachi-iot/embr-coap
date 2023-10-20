#include "context.h"

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>


using namespace embr::coap;

void AppContext::populate_uri_int(const event::option& e)
{
    // DEBT: As is the case all over, 'chunk' is assumed to be complete
    // data here
    auto option = (const char*) e.chunk.data();
    
    if(estd::from_chars(option, option + e.chunk.size(), *uri_int).ec == 0)
        ESP_LOGD(TAG, "Selecting gpio # %d", *uri_int);
}

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
                    populate_uri_int(e);
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

            /* from_query already does this, but I think I might prefer it here
            const estd::string_view value = estd::get<1>(q);

            ESP_LOGV(TAG, "on_notify: uri-query key=%.*s value=%.*s",
                key.size(), key.data(),
                value.size(), value.data());    */

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
