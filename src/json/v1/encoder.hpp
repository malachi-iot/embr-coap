#pragma once

#include "encoder.h"

// DEBT: For endl, but endl itself should have a declaration in iosfwd
#include <estd/ostream.h>

namespace embr { namespace json {

namespace v1 {

template <class TStreambuf, class TBase>
inline void encoder::do_eol(estd::internal::basic_ostream <TStreambuf, TBase>& out)
{
    if (use_eol()) out << estd::endl;
}

}

}}