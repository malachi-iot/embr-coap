/**
 * @file
 */
#pragma once

#include "events.h"
#include "factory.h"
#include <estd/optional.h>

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

struct CoREData
{
    int node;
    estd::layer2::const_string resource_type;
    estd::layer2::const_string interface_description;
};

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

inline const UriPathMap* match(const UriPathMap* items, int count, estd::layer3::const_string s, int parent_id)
{
    while(count--)
    {
        const UriPathMap* candidate = items++;

        if(s == candidate->first && candidate->third == parent_id)
        {
            return candidate;
        }
    }

    return NULLPTR;
}

struct UriPathMatcherBase
{
    typedef typename estd::layer1::optional<int, MCCOAP_URIPATH_NONE> optional_int_type;

    ///
    /// \param uri_piece
    /// \param within which parent uri group to search in
    /// \param start_from slight abuse, we pass back how far we got via this also
    /// \param end end of search items.  mainly used to make this a static call
    /// \return id of node matching uri_piece + within, or MCCOAP_URIPATH_NONE
    /// \remarks identical to preceding 'match' operation
    template <class TIterator>
    static optional_int_type find(
            estd::layer3::const_string uri_piece, int within,
            TIterator& start_from,
            const TIterator& end)
    {
        typedef typename estd::remove_reference<TIterator>::type iterator;

#ifdef FEATURE_CPP_STATIC_ASSERT
        /* doesn't work because iterator can be UriPathMap*
        static_assert (estd::is_same<typename iterator::value_type, UriPathMap>::value,
                       "Requires iterator which reflects a UriPathMap"); */
#endif
        // NOTE: this means if we don't find anything, our iterator is all the way at the end
        // which may actually not be what we want
        iterator& i = start_from;

        for(;i != end; i++)
        {
            if(uri_piece == (*i).first && (*i).third == within)
            {
                int node_id = (*i).second;
                return node_id;
            }
        }

        return estd::nullopt;
    }

};



template <class TEvent, class TSubject, class TIterator>
///
/// \brief iterate from begin to end and notify subject for each element iterated over
/// \param subject
/// \param begin
/// \param end
///
void iterate_and_notify(TSubject& subject,
                        const TIterator& begin,
                        const TIterator& end)
{
    for(TIterator i = begin;i != end; i++)
        subject.notify(TEvent(*i));
}

template <class TEvent, class TSubject, class TContainer>
///
/// \brief notify subject for each element in container
/// \param subject
/// \param container
///
void iterate_and_notify(TSubject& subject, const TContainer& container)
{
    iterate_and_notify<TEvent>(subject, container.begin(), container.end());
}

// kind of like a multimap
// incoming container expected to be sorted based primarily on
// parent id ('third')
// NOTE: designed to be used after the container has been created,
// and MAYBE to help track state when retrieving the URI
template <class TContainer,
          class TProviderBase = estd::experimental::instance_provider<const TContainer> >
struct UriPathMatcher2 :
        UriPathMatcherBase,
        TProviderBase
{
    typedef UriPathMatcherBase base_type;
    typedef TProviderBase provider_type;

    //TContainer container;

    typedef typename estd::remove_reference<TContainer>::type container_type;
    typedef typename container_type::iterator iterator;
    typedef typename container_type::value_type value_type;

    //container_type& container() { return provider_type::value(); }
    const container_type& container() const { return provider_type::value(); }

#ifdef FEATURE_CPP_STATIC_ASSERT
    static_assert (estd::is_same<
                   typename estd::remove_const<value_type>::type,
                   UriPathMap>::value, "Expect a container of UriPathMap");
#endif

    // probably *do not* want to track this state in here, but just for
    // experimentation gonna toss it in
    optional_int_type last_found;
    iterator last_pos;

    template <class TParam1>
    UriPathMatcher2(const TParam1& p1) :
        provider_type(p1),
        //container(p1),
        last_pos(container().begin())
    {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    UriPathMatcher2(container_type&& container) :
        provider_type(std::move(container)),
        //container(std::move(container)),
        last_pos(container().begin())
        {}
#endif

    // this is 'standalone' one and doesn't leverage tracked state
    // FIX: confusing, easy to call wrong one
    optional_int_type find(estd::layer3::const_string uri_piece, int within) const
    {
        iterator i = container().begin();
        return base_type::find(uri_piece, within, i, container().end());
    }

    ///
    /// \brief stateful search
    /// \param uri_piece
    /// \return
    optional_int_type find(estd::layer3::const_string uri_piece)
    {
        // NOTE: this optional behavior will mean if it can't find the uri_piece,
        // it's going to restart since !has_value means MCCOAP_URIPATH_NONE, which
        // is used by root (i.e. v1) paths
        return last_found = base_type::find(uri_piece,
                                 last_found ? last_found.value() : MCCOAP_URIPATH_NONE,
                                 last_pos, container().end());
    }


    // do a SAX-like notification walk over all known nodes
    // (useful for CoRE processing)
    // not 100% required for container to be in order, but highly recommended
    template <class TSubject>
    void notify(TSubject& subject) const
    {
        iterate_and_notify<known_uri_event>(subject, container());

        /*
        iterator i = container().begin();

        for(;i != container().end(); i++)
        {
            const value_type& v = *i;

            subject.notify(known_uri_event(v));
        } */
    }
};




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
