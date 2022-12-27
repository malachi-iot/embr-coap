#pragma once

#include "subject.h"

namespace embr { namespace coap {

namespace experimental { namespace observable { namespace lwip {

template <typename F>
void Notifier::notify(registrar_type& registrar, handle_type handle, F&& f)
{
    registrar_type::container_type::iterator i = registrar.observers.begin();

    for(; i != registrar.observers.end(); ++i)
    {
        // DEBT: Pretty sure this is std preferred way, but look into arrow
        // operator here
        if((*i).handle == handle)
        {
            const registrar_type::key_type& key = *i;

            encoder_factory::encoder_type e = encoder_factory::create();

            Header header(Header::NonConfirmable);

            header.token_length(key.token.size());
            // DEBT: Not likely we'll always want a PUT here
            header.code(Header::Code::Put);

            e.header(header);
            // DEBT: Seems like we could pass a token directly in here
            e.token(key.token.data(), key.token.size());
        }
    }
}

}}}

}}
