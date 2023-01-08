#pragma once

#include "../../internal/rfc7641/keys.h"

namespace embr { namespace coap {

namespace internal {

template <class TContainer, observable::detail::SequenceTracking st, class TContext, typename enable =
    typename estd::enable_if<
        estd::is_base_of<tags::token_context, TContext>::value &&
        estd::is_base_of<tags::address_context, TContext>::value
    >::type
>
Header::Code add_or_remove(
    observable::detail::Registrar<TContainer, st>& registrar,
    TContext& context,
    observable::option_value_type option_value,
    int resource_id)
{
    return registrar.add_or_remove(
        option_value.value(),
        context.address(),
        context.token().span(),     // DEBT: Token needs rework
        resource_id);
}


struct ObservableObserver
{
    static void on_notify(const event::option& e, embr::coap::internal::ExtraContext& context)
    {
        // DEBT: Only accounts for request/GET mode
        // need to account also for notification receipt (i.e. sequence number)
        if(e.option_number == Option::Observe)
        {
            uint16_t value = UInt::get<uint16_t>(e.chunk);
            context.flags.observable = (observable::Options)value;
        }
    }

// Really nifty code, but alas we don't want to observe EVERY request
#if UNUSED
    template <class TContext, typename enable =
        typename estd::enable_if<
            estd::is_base_of<tags::token_context, TContext>::value &&
            estd::is_base_of<tags::address_context, TContext>::value &&
            estd::is_base_of<embr::coap::internal::ExtraContext, TContext>::value &&
            estd::is_base_of<tags2::notifier_context, TContext>::value
        >::type
    >
    static void on_notify(event::option_completed, TContext& context)
    {
        Header::Code::Codes code = Header::Code::NotImplemented;

        code = context.notifier().add_or_remove(
            context.observe_option().value(),
            context.address(),
            context.token(),
            context.found_node());

        if(code != Header::Code::Valid)
        {
            context.flags.observable = experimental::observable::Unspecified;
        }
    }
#endif
};

}

}}