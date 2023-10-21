#include "context.h"
#include "from_query.h"

// DEBT: Needed for encoder/streambuf.h finalize to compile -
// fix this because we aren't even calling finalize here
#include <coap/platform/lwip/encoder.h>


using namespace embr::coap;


void AppContext::populate_uri_int(const event::option& e)
{
    // DEBT: As is the case all over, 'string' is assumed to be complete
    // data here
    if(from_string(e.string(), *uri_int).ec == 0)
        ESP_LOGV(TAG, "Found uri int=%d", *uri_int);
    else
        ESP_LOGD(TAG, "Was expecting uri int, but found none");
}

bool AppContext::on_notify(const event::option& e)
{
    switch(e.option_number)
    {
        case Option::UriPath:
        {
            const int node = found_node();

            if(node >= 0)
            {
                const embr::coap::internal::UriPathMap* current = uri_matcher().current();

                ESP_LOGV(TAG, "on_notify(option)=uri-path matched=%s",
                    current->first.data());

                if(current->first[0] == '*')
                {
                    populate_uri_int(e);
                }
            }

            // DEBT: Not available, think we'd like to expose ::types - though
            // really we do want visitor
            //using state_types = decltype(state)::types;
            //using state_visitor = decltype(state)::visitor;
            // DEBT: Would really like to use 'Substate' here for T but it's not playin gnice
            // DEBT: 'visit' naming here a little dubious, because we usually have no business visiting
            // all the instances in a variant.  This is an exceptional case.  Perhaps we should rename
            // it to 'visit_instance' to reflect that? - just as variant_storage does
            // We aren't actually using the instance here anyway
            state.visit([this, node]<unsigned I, class T>(estd::variadic::v2::instance<I, T> vi)
            {
                // All that above DEBT aside, we have a badass emplace factory!
                if(node != T::id_path) return false;

                state.emplace<T>(*this);
                return true;
            });
            break;
        }

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


bool AppContext::on_completed(encoder_type& encoder)
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
        {
            //completed_gpio(encoder);
            const Header::Code c = estd::get<states::gpio>(state).completed(encoder);
            // DEBT: Slightly clumsy this handling of response code
            if(c != Header::Code::Empty)    response_code = c;
            break;
        }

        case id_path_v1_api_pwm:
            response_code = estd::get<states::ledc_timer>(state).completed(encoder);
            break;

        case id_path_v1_api_pwm_value:
            //completed_ledc_channel(encoder);
            response_code = estd::get<states::ledc_channel>(state).completed(encoder);
            break;

        default:    return false;
    }

    return true;
}


bool AppContext::on_payload(istreambuf_type& payload)
{
    istream_type in(payload);

    switch(found_node())
    {
        case id_path_v1_api_gpio_value:
            estd::get<states::gpio>(state).on_payload(in);
            break;

        case id_path_v1_api_pwm_value:
            estd::get<states::ledc_channel>(state).on_payload(in);
            break;

        default:    return false;
    }

    return true;
}
