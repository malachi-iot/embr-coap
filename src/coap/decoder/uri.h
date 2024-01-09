/**
 * @file
 * Where pattern matching / uri parsing (for decoding specifically) code lives
 */
#pragma once

#include <estd/optional.h>
#include <estd/string.h>


namespace embr { namespace coap { namespace internal {

#define MCCOAP_URIPATH_NONE -1
#define MCCOAP_URIPATH_UNSPECIFIED -2


// Would have used tuple, but this way is C++03 friendly
template<class T1, class T2, class T3>
struct triad
{
    T1 first;
    T2 second;
    T3 third;
};

//typedef estd::pair<const char*, int> UriPathMap;
// uri part, id of this uri, id of parent uri
// 09JAN24 I think we can liberate this guy from 'internal' soon
typedef triad<estd::layer2::const_string, int, int> UriPathMap;


struct UriPathMatcher
{
    typedef estd::layer1::optional<int, MCCOAP_URIPATH_NONE> optional_int_type;

    const UriPathMap* _current;
    const UriPathMap* const _end;
    optional_int_type parent_id;

    template <size_t N>
    UriPathMatcher(const UriPathMap (&paths)[N]) :
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
            if(_current->third == *parent_id)
            {
                // wildcard, for use with parameterized URIs
                // DEBT: Further parameters on URI options are undefined i.e.
                // "coap://v1/api/gpio/0" works but
                // "coap://v1/api/gpio/0/true" is undefined
                if(_current->first == "*")
                {
                    parent_id = (int)_current->second;
                    return _current;
                }
                // if evaluated node then matches path segment
                else if(_current->first == uripath)
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

    ESTD_CPP_CONSTEXPR_RET const optional_int_type& last_found() const
    {
        return parent_id;
    }

    ESTD_CPP_CONSTEXPR_RET const UriPathMap* current() const
    {
        return _current;
    }
};

}}}
