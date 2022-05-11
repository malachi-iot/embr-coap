#pragma once

// DEBT: needed for some fixed-buf allocator defs
#include <estd/array.h>

#include "../coap/decoder/uri.h"

// embr subject/observer version of uripath handlers
// hangs off new embr-based decoder subject
namespace embr { namespace coap {

namespace experimental {

typedef embr::coap::internal::UriPathMap UriPathMap;

inline const UriPathMap* match(const UriPathMap* items, int count, estd::layer3::const_string s, int parent_id)
{
    while (count--)
    {
        const UriPathMap* candidate = items++;

        if (s == candidate->first && candidate->third == parent_id)
        {
            return candidate;
        }
    }

    return NULLPTR;
}



}}}

