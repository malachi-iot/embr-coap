#pragma once

#include "notifier.h"

#include "../../../internal/rfc7641/registrar.h"
#include "../../../internal/rfc7641/notifier.hpp"

namespace embr { namespace coap {

namespace internal { namespace observable { namespace lwip {

template <typename TContainer, observable::detail::SequenceTracking st, typename F>
inline void Notifier2::notify(coap::internal::observable::detail::Registrar<TContainer, st>& registrar, handle_type handle,
    embr::lwip::udp::Pcb pcb, F&& f)
{
    typedef detail::Registrar<TContainer, st> registrar_type;
    typedef typename registrar_type::const_reference ref;

    return notify(registrar, pcb, [=](ref key)
    {
        return key.handle == handle;
    }, std::move(f));
}


template <typename TContainer, observable::detail::SequenceTracking st, typename P, typename F>
void Notifier2::notify(coap::internal::observable::detail::Registrar<TContainer, st>& registrar, 
    embr::lwip::udp::Pcb pcb, P&& predicate, F&& f)
{
    typedef detail::Registrar<TContainer, st> registrar_type;
    typedef typename registrar_type::container_type container_type;

    // DEBT: Need a proper message id generator, but this will do
    // in the short term.  This generator ideally tracks mids used with a particular
    // endpoint to avoid deduplication
    static unsigned message_id = 0;

    typename container_type::iterator i = registrar.observers.begin();

    for(; i != registrar.observers.end(); ++i)
    {
        if(predicate(*i))
        {
            const typename registrar_type::key_type& key = *i;

            encoder_factory::encoder_type e = encoder_factory::create();

            Header header(Header::NonConfirmable);

            header.token_length(key.token.size());
            // DEBT: Not likely we'll always want a Content here
            header.code(Header::Code::Content);
            header.message_id(++message_id);

            // DEBT: Need to put sequence option in here ... possibly

            e.header(header);
            e.token(key.token);

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
