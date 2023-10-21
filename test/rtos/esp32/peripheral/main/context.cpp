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
                const char* match_uri = current->first.data();

                ESP_LOGV(TAG, "on_notify(option)=uri-path matched=%s", match_uri);

                if(match_uri[0] == '*')
                    populate_uri_int(e);
            }

            state.create(node, *this);

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


bool AppContext::on_completed(encoder_type& encoder)
{
    return state.on_completed(encoder, *this);

    /*
    return state.state().visit_index([&](auto i)
    {
        Header::Code code = i->response();

        if(code == Header::Code::Empty)
            return i->completed(encoder);
        else
            response_code = code;

        return true;
    }); */
}
