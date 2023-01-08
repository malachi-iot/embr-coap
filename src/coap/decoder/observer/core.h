#pragma once

#include "util.h"
#include "observable.h"
#include "uri.h"

#include <embr/observer.h>

namespace embr { namespace coap {

namespace internal {

// DEBT: Name is awful
struct ExtraObserver
{
    static void on_notify(event::tags::payload, internal::ExtraContext& context)
    {
        context.flags.payload = true;
    }
};

// NOTE: The notion of a 'CoreContext' is complicated because so far we depend a lot on LwipIncomingContext,
// which would largely interfere with a 'CoreContext'

}

// DEBT: Consider a generic aggregated observer which we typedef here
struct CoreObserver
{
    // Remember, inheritance doesn't find on_notify with our notify mechanism,
    // so rewire and effectively inherit here
    typedef embr::layer0::subject<
        TokenContextObserver,
        HeaderContextObserver,
        UriParserObserver,
        internal::ObservableObserver,
        internal::ExtraObserver> aggregate_type;

    template <class TEvent, class TContext>
    static void on_notify(const TEvent& e, TContext& c)
    {
        aggregate_type().notify(e, c);
    }
};

}}