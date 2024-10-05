/**
 * @file
 */
#pragma once

#include "events.h"
#include "factory.h"
#include <estd/optional.h>
#include <estd/ostream.h>
#include <estd/stack.h>

#include <estd/sstream.h>
#include <estd/internal/ostream_basic_string.hpp>
#include <estd/optional.h>

#include <embr/observer.h>

#include "uripathmap.h"

// embr subject/observer version of uripath handlers
// hangs off new embr-based decoder subject
namespace embr { namespace coap {

namespace experimental {

typedef estd::pair<const char*, int> UriPathMap2;

// various Link Target Attributes are available, outlined here
// https://tools.ietf.org/html/rfc6690#section-2 and also ;ct= for content type
struct CoREData
{
    int node;
    estd::layer2::const_string resource_type;
    estd::layer2::const_string interface_description;

    // "65000-65535 are reserved for experiments"
    // if we keep this internal meaning NULL, should be acceptable use case
    estd::layer1::optional<uint16_t, 65000> content_type;
    void* test_empty;
    // NOTE: has no default constructor, and probably shouldn't - but would
    // be convenient for this use case
    //estd::layer2::const_string test;
};


template <class TValue = estd::layer2::const_string>
struct CoREData2Item
{
    typedef TValue value_type;

    int node;
    TValue value;
};

// this experimental approach goes vertically, database style and we pick up
// nodes along the way
// since it's an aggregate of the same particular attribute, TCollection could be
// an array, vector, etc.
template <class TCollection>
struct CoREData2
{
    // Link Target Attribute name (ala https://tools.ietf.org/html/rfc6690#section-7.4)
    estd::layer2::const_string attribute;

    TCollection items; // should be a collection of CoREData2Item
};


template <class TValue = estd::layer2::const_string>
struct CoreData3Item
{
    // Link Target Attribute name (ala https://tools.ietf.org/html/rfc6690#section-7.4)
    // Since everything's so templaty-strongly typed anyway maybe this can become a
    // compile time constant
    estd::layer2::const_string attribute;

    TValue value;
};

// this experimental approach goes horizontally
// in this case TCollection will probably have to be a tuple
template <class TCollection>
struct CoreData3
{
    int node;
    TCollection attributes;
};

// NOTE: What might be best is a simpler registry of CoRE responders to an output stream
// invoked per service.  That was kind of my original idea, very C# like.  Like this:
struct CoREResponder
{
    virtual void generate(estd::layer3::string output) = 0;
};
// ^^ it's cleaner but we're gonna get a lot of virtual function overhead.  Especially because
// cramming to estd::layer3::string is not a good long term solution, we'll need to map to
// netbuf and/or ostream


template <class TStreambuf, class TBase>
///
/// \brief emits CoRE specific formatted link suffix
///
/// Conformant to https://tools.ietf.org/html/rfc6690#section-5
///
/// \param out
/// \param value
/// \return
///
estd::detail::basic_ostream<TStreambuf, TBase>& operator<<(
        estd::detail::basic_ostream<TStreambuf, TBase>& out,
        const CoREData& value
        )
{
    if(!value.resource_type.empty())
        out << ";rt=\"" << value.resource_type << '"';

    if(!value.interface_description.empty())
        out << ";if=\"" << value.interface_description << '"';

    return out;
}

struct UriPathMatcher
{
    estd::layer3::const_string match_to;

    void on_notify(const event::uri_path& e)
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

    const estd::layer2::const_string& uri_part() const { return path_map.first; }
    int node_id() const { return path_map.second; }
    int parent_id() const { return path_map.third; }

    known_uri_event(const UriPathMap& path_map) : path_map(path_map) {}
};

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

    // breadcrumb tracker for hierarchical navigation of nodes
    // NOTE: inactive here, just a proof of concept but seems listeners from notify/stateful
    // tracking of where we are in the hierarchy would be interesting to most listeners
    estd::layer1::stack<const UriPathMap*, 10> parents;



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

inline internal::triad<int, UriPathMap2*, int>* match(internal::triad<int, UriPathMap2*, int>* items, int count,
                                            int key)
{
    while(count--)
    {
        internal::triad<int, UriPathMap2*, int>* candidate = items++;

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
    void on_notify(const event::option& e)
    {
        //estd::string_view s;
        estd::string_view uri_path((const char*)e.chunk.data(),
                                   e.chunk.size());

        event::uri_path upe { uri_path };

        subject.on_notify(upe);
    }
};



template <class TStream>
struct ostream_event
{
#ifdef FEATURE_CPP_STATIC_ASSERT
    // TODO: assert that we really are getting an ostream here
#endif

    TStream& ostream;

    ostream_event(TStream& ostream) : ostream(ostream) {}
};



template <class TStream>
///
/// \brief Event for evaluating a well-known CoRE request against a particular service node
///
/// In particular this event is used for producing specialized output for the distinct URI node
/// reported in the CoRE response (i.e. you can use ostream to spit out ';title="something"' or
/// similar
///
/// Also has experimental helpers (title, content_type, size) which further compound the unusual
/// aspect of semi-writing-to an event
///
struct node_core_event : ostream_event<TStream>
{
    const int node_id;
    typedef ostream_event<TStream> base_type;

    node_core_event(int node_id, TStream& ostream) :
        base_type(ostream),
        node_id(node_id)
    {}

    template <class T>
    void title(const T& value) const
    {
        base_type::ostream << ";title=\"" << value << '"';
    }

    void content_type(unsigned value) const
    {
        base_type::ostream << ";ct=" << value;
    }

    void size(unsigned value) const
    {
        base_type::ostream << ";sz=" << value;
    }
};


struct title_tacker
{
    // FIX: May be slight abuse because events are supposed to be read only,
    // but this ostream use isn't quite that.  Works, though
    template <class TStream>
    void on_notify(const node_core_event<TStream>& e)
    {
        e.title("test");
        // FIX: As is the case also with SAMD chips, the numeric output is broken
        //e.size(e.node_id * 100);
    }
};




// foundational/poc for emitter of CoRE for /.well-known/core responder
// mainly has advantage of decoupling programmer from fiddling with specific
// TOStream types
struct core_evaluator
{
    // all uripath nodes which actually have CoRE data associated
    // with them
    // TODO: A map would be better instead of a layer3 array
    typedef estd::legacy::layer3::array<CoREData, uint8_t> coredata_type;
    coredata_type coredata;

    // breadcrumb tracker for hierarchical navigation of nodes
    estd::layer1::stack<const UriPathMap*, 10> parents;

    core_evaluator(coredata_type coredata) :
        coredata(coredata) {}

    void parent_handler(const known_uri_event& e)
    {
        // if the last parent id we are looking at doesn't match incoming
        // observed one, back it out until it does match the observed one
        while(!parents.empty() && parents.top()->second != e.parent_id())
            parents.pop();

        // when we arrive here, we're either positioned in 'parents' at the
        // matching parent OR it's empty
        parents.push(&e.path_map);
    }

    template <class TOStream>
    // NOTE: Does not include delimiter
    // returns true if this node actually is present in coredata list
    bool
    //estd::optional<int>  // FIX: estd::optional int broken, can't assign int to it
    evaluate(const known_uri_event& e, TOStream& out)
    {
        //typedef typename estd::remove_reference<TOStream>::type ostream_type;
        // 'top' aka the back/last/current parent

        parent_handler(e);

        // TODO: Need a way to aggregate CoRE datasources here
        // and maybe some should be stateful vs the current semi-global
        const auto& result = std::find_if(coredata.begin(), coredata.end(),
                                    [&](const CoREData& value)
        {
            return value.node == e.node_id();
        });

        // only spit out CoRE results for nodes with proper metadata.  Eventually
        // maybe we can auto-deduce based on some other clues reflecting that we
        // service a particular uri-path chain
        if(result != coredata.end())
        {
            out << '<';

            // in atypical queue/stack usage we push to the back but evaluate (but not
            // pop) the front/bottom.  This way we can tack on more and more uri's to reflect
            // our hierarchy level, then pop them off as we leave
            // NOTE: our layer1::stack can do this but the default stack cannot
#ifdef FEATURE_CPP_RANGED_FORLOOP
            for(const auto& parent_node : parents)
                out << '/' << parent_node->first;
#else
#error Not yet supported
#endif

            out << '>' << *result;

            //return result->node;
            return true;
        }

        //return estd::nullopt;
        return false;
    }
};

template <class TOStream = estd::experimental::ostringstream<256> >
struct core_observer : core_evaluator
{
    core_observer(coredata_type coredata) :
        core_evaluator(coredata) {}

    typedef TOStream ostream_type;

    // in non-proof of concept, this won't be inline with the observer
    ostream_type out;

    // NOTE: doing this via notify but perhaps could easily do a more straight
    // ahead sequential version
    void on_notify(const known_uri_event& e)
    {
        // if false, this node isn't a CoRE participator
        if(!evaluate(e, out)) return;

        // tack on other link attributes
        node_core_event<ostream_type> _e(e.node_id(), out);

        auto link_attribute_evaluator2 = embr::layer1::make_subject(title_tacker());
        link_attribute_evaluator2.notify(_e);

        // TODO: We'll need to intelligently spit out a comma
        // and NOT endl unless we specifically are in a debug mode
        out << estd::endl;
    }
};

}

}}
