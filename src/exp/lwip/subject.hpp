#pragma once

#include "subject.h"

namespace embr { namespace coap {

namespace experimental { namespace observable { namespace lwip {

template <typename F>
void Notifier::notify(registrar_type& registrar, handle_type handle,
    embr::lwip::udp::Pcb pcb, F&& f)
{
    // DEBT: Need a proper message id generator, but this will do
    // in the short term.  This generator ideally tracks mids used with a particular
    // endpoint to avoid deduplication
    static unsigned message_id = 0;

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
            // DEBT: Not likely we'll always want a Content here
            header.code(Header::Code::Content);
            header.message_id(++message_id);

            // DEBT: Need to put sequence option in here ... possibly

            e.header(header);
            // DEBT: Seems like we could pass a token directly in here
            e.token(key.token.data(), key.token.size());

            f(key, e);

            // DEBT: I'd still prefer a bit more automatic/obvious call
            // it's easy to forget the finalize
            e.finalize();

            pcb.send_experimental(e.rdbuf()->pbuf(), key.endpoint);
        }
    }
}

}}}

}}
