// See README.md
#pragma once

#include <estd/cstdint.h>
#include <embr/scheduler.h>

namespace embr { namespace coap { namespace experimental { namespace retry {

template <class TClock>
struct _Scheduler
{
    typedef TClock clock_type;
    typedef typename clock_type::time_point time_point;

    // DEBT: We'd prefer a passed in Scheduler
    embr::internal::layer1::Scheduler<8, embr::internal::scheduler::impl::Function<time_point> > scheduler;
};

}}}}