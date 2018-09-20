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
