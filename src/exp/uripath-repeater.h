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
typedef triad<const char*, int, int> UriPathMap;

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
