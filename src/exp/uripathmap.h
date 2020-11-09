#pragma once

#include <estd/string.h>

// embr subject/observer version of uripath handlers
// hangs off new embr-based decoder subject
namespace moducom { namespace coap {

namespace experimental {

#define MCCOAP_URIPATH_NONE -1
#define MCCOAP_URIPATH_UNSPECIFIED -2

template<class T1, class T2, class T3>
struct triad
{
    T1 first;
    T2 second;
    T3 third;
};

//typedef estd::pair<const char*, int> UriPathMap;
// uri part, id of this uri, id of parent uri
typedef triad<estd::layer2::const_string, int, int> UriPathMap;

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

