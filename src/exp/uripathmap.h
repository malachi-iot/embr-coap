#pragma once

// DEBT: needed for some fixed-buf allocator defs
#include <estd/array.h>

#include <estd/optional.h>
#include <estd/string.h>

// embr subject/observer version of uripath handlers
// hangs off new embr-based decoder subject
namespace embr { namespace coap {

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

struct UriPathMatcher3
{
    typedef estd::layer1::optional<int, MCCOAP_URIPATH_NONE> optional_int_type;

    const UriPathMap* _current;
    const UriPathMap* const _end;
    optional_int_type parent_id;

    template <size_t N>
    UriPathMatcher3(const UriPathMap (&paths)[N]) :
        _current(paths),
        _end(&paths[N])
    {
    }

    // find, under currently tracked parent, node id/UriPathMap* associated with uripath
    const UriPathMap* find(estd::layer3::const_string uripath)
    {
        for(;_current != _end; _current++)
        {
            // evaluated node has to match parent_id
            // NOTE: very slight abuse, .value() used even when 'no value'
            // of -1
            if(_current->third == parent_id.value())
            {
                // if evaluated node then matches path segment
                if(_current->first == uripath)
                {
                    // new parent becomes this node
                    // cast to value needed because optional takes move semantic
                    parent_id = (int)_current->second;
                    return _current;
                }

                // if avaluated node does not match path segment,
                // continue looking
            }
            else
            {
                // if avaluated node parent does not match, continue looking
            }
        }

        parent_id.reset();
        return NULLPTR;
    }

    const optional_int_type& last_found() const
    {
        return parent_id;
    }
};

}}}

