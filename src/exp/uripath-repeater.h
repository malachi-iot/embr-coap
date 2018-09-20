/**
 * @file
 */
#pragma once

#include "events.h"
#include "factory.h"

// embr subject/observer version of uripath handlers
// hangs off new embr-based decoder subject
namespace moducom { namespace coap {

namespace experimental {

#define MCCOAP_URIPATH_NONE -1
#define MCCOAP_URIPATH_UNSPECIFIED -2

template <class T1, class T2, class T3>
struct triad
{
    T1 first;
    T2 second;
    T3 third;
};

//typedef estd::pair<const char*, int> UriPathMap;
// uri part, id of this uri, id of parent uri
typedef triad<estd::layer2::const_string, int, int> UriPathMap;

typedef estd::pair<const char*, int> UriPathMap2;

struct UriPathMatcher
{
    estd::layer3::const_string match_to;

    void on_notify(const uri_path_event& e)
    {
        if(e.path == match_to)
        {

        }
    }
};

// FIX: needs better name. used for iteration over known nodes from
// uripathmatcher
struct known_uri_event
{
    //estd::layer3::string uri_part;
    const UriPathMap& path_map;

    known_uri_event(const UriPathMap& path_map) : path_map(path_map) {}
};

// kind of like a multimap
// incoming container expected to be sorted based primarily on
// parent id ('third')
// NOTE: designed to be used after the container has been created,
// and MAYBE to help track state when retrieving the URI
template <class TContainer>
struct UriPathMatcher2
{
    TContainer container;

    typedef typename estd::remove_reference<TContainer>::type container_type;
    typedef typename container_type::iterator iterator;
    typedef typename container_type::value_type value_type;

    // probably *do not* want to track this state in here, but just for
    // experimentation gonna toss it in
    int last_found;

    template <class TParam1>
    UriPathMatcher2(TParam1& p1) :
        container(p1),
        last_found(MCCOAP_URIPATH_NONE)
    {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    UriPathMatcher2(container_type&& container) :
        container(std::move(container)),
        last_found(MCCOAP_URIPATH_NONE)
        {}
#endif

    int find(estd::layer3::const_string uri_piece, int within)
    {
        iterator i = container.begin();

        for(;i != container.end(); i++)
        {
            if(uri_piece == (*i).first && (*i).third == within)
            {
                return (*i).second;
            }
        }
    }

    int find(estd::layer3::const_string uri_piece)
    {
        return last_found = find(uri_piece, last_found);
    }


    // do a SAX-like notification walk over all known nodes
    // (useful for CoRE processing)
    // not 100% required for container to be in order, but highly recommended
    template <class TSubject>
    void notify(TSubject& subject)
    {
        iterator i = container.begin();

        for(;i != container.end(); i++)
        {
            const value_type& v = *i;

            subject.notify(known_uri_event(v));
        }
    }
};


inline UriPathMap* match(UriPathMap* items, int count, estd::layer3::const_string s, int parent_id)
{
    while(count--)
    {
        UriPathMap* candidate = items++;

        if(s == candidate->first && candidate->third == parent_id)
        {
            return candidate;
        }
    }

    return NULLPTR;
}


inline int match(UriPathMap2* items, int count, estd::layer3::const_string s)
{
    while(count--)
    {
        UriPathMap2* candidate = items++;

        if(s == candidate->first)
        {
            return candidate->second;
        }
    }

    return MCCOAP_URIPATH_NONE;
}

inline triad<int, UriPathMap2*, int>* match(triad<int, UriPathMap2*, int>* items, int count,
                                            int key)
{
    while(count--)
    {
        triad<int, UriPathMap2*, int>* candidate = items++;

        if(candidate->first == key) return candidate;
    }

    return NULLPTR;
}


// rebroadcasts events sourced either from decoder subject or
// a parent UriPathRepeater itself
// for the time being we are going to assume that all incoming options *ARE NOT*
// chunked.  This will need to be revisited
template <class TSubject>
struct UriPathRepeater
{
    // subject is who we will repeat to
    TSubject subject;

    // received from Decoder Subject, if one is broadcasting to us
    void on_notify(const option_event& e)
    {
        //estd::string_view s;
        estd::string_view uri_path((const char*)e.chunk.data(),
                                   e.chunk.size());

        uri_path_event upe { uri_path };

        subject.on_notify(upe);
    }
};


}

}}
