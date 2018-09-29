#include <catch.hpp>

#include "exp/misc.h"

#include <embr/datapump.hpp>
#include <embr/observer.h>
#include <embr/netbuf-static.h>

#include <exp/retry.h>

#include <exp/message-observer.h>
#include "test-observer.h"

#include "exp/prototype/observer-idea1.h"

#include "coap/decoder/subject.hpp"
#if __cplusplus >= 201103L
#include "platform/generic/malloc_netbuf.h"
#endif

#include "exp/events.h"
#include <exp/uripath-repeater.h>

//#include <estd/map.h>
#include <estd/sstream.h>
#include <estd/internal/ostream_basic_string.hpp>

#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
typedef moducom::pipeline::MemoryChunk chunk_t;


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


TEST_CASE("experimental 2 tests")
{
    typedef uint32_t addr_t;


#ifdef FEATURE_CPP_VARIADIC
    SECTION("AggregateMessageObserver")
    {
        moducom::pipeline::layer1::MemoryChunk<256> chunk;

        ObserverContext ctx(chunk);
        AggregateMessageObserver<ObserverContext,
                Buffer16BitDeltaObserver<ObserverContext>,
                EmptyObserver<ObserverContext>
                > amo(ctx);

        Header test_header(Header::Confirmable);
        //moducom::coap::layer1::Token token = ;
        uint8_t raw_token[] = { 0x45, 0x67 };

        test_header.token_length(sizeof(raw_token));
        test_header.message_id(0x0123);

        amo.on_header(test_header);
        //amo.on_token(raw_token); // not doing this only because Buffer16BitDeltaObserver hates it
        amo.on_option((moducom::coap::Option::Numbers) 270, 1);
    }
#endif
    SECTION("prototype observer idea1")
    {
        using namespace ::experimental::prototype::idea1;

#ifdef FEATURE_CPP_LAMBDA
        DecoderObserver test_target1([]()
        {
            // eventually activated when v1/target1 get is issued
        });
#endif

        URI uri[] =
        {
#if defined(FEATURE_CPP_MOVESEMANTIC) && defined(FEATURE_CPP_LAMBDA)
            URI("v1",
            {
                URI("target1", test_target1, true),
                URI("target2", [](IncomingContext<int>& ctx)
                {
                    // eventually activated when v2/target2 get is issued
                })
            }),
#endif
            URI("v2", 0, 2)
        };

        IncomingContext<int> ctx;

        decoder_observer(DecoderObserver(uri), ctx, buffer_16bit_delta);
    }
    SECTION("RandomPolicy test")
    {
        int value;

        for(int i = 0; i < 10; i++)
        {
            value = RandomPolicy::rand(100, 200);

            REQUIRE(value >= 100);
            REQUIRE(value <= 200);
        }
    }
    SECTION("uripath repeater")
    {
        moducom::coap::experimental::UriPathRepeater<embr::void_subject> upr;
    }
    SECTION("factory test")
    {
        constexpr int id_path_v1 = 0;
        constexpr int id_path_v1_api = 1;
        constexpr int id_path_v1_api_power = 2;
        constexpr int id_path_v1_api_test = 6;
        constexpr int id_path_dummy = 3;
        constexpr int id_path_v2 = 4;
        constexpr int id_path_v2_api = 5;
        constexpr int id_path_well_known = 7;
        constexpr int id_path_well_known_core = 8;

        // sort by parent id first, then by node id
        // this way we can easily optimize incoming request parsing by remembering
        // where we left off on found node id position
        // NOTE: might be better to sort by uri_path secondly instead of node_id,
        // as it potentially could speed up searching for said path.  However,
        // any of the secondary sorting the benefit is limited by our ability to know
        // how large the 'parent' region is in which we are sorting, which so far
        // is elusive
        const UriPathMap map[] =
        {
            { "v1",     id_path_v1,             MCCOAP_URIPATH_NONE },
            { "api",    id_path_v1_api,         id_path_v1 },
            { "power",  id_path_v1_api_power,   id_path_v1_api },
            { "test",   id_path_v1_api_test,    id_path_v1_api },

            { "v2",     id_path_v2,             MCCOAP_URIPATH_NONE },
            { "api",    id_path_v2_api,         id_path_v2 },

            { ".well-known",    id_path_well_known,         0 },
            { "core",           id_path_well_known_core,    id_path_well_known }
        };

        // NOTE: Not favoring this discrete UriPathMap2 method, it's a little verbose
        UriPathMap2 map_root[] =
        {
            { "v1", id_path_v1 }
        };

        UriPathMap2 map_v1[] =
        {
            { "api", id_path_v1_api }
        };

        UriPathMap2 map_v1_api[] =
        {
            { "power", id_path_v1_api_power }
        };

        // first = parent
        // second = list of children
        // third = count of children
        triad<int, UriPathMap2*, int> relations[]
        {
            { MCCOAP_URIPATH_NONE,  map_root, 1 },
            { id_path_v1,           map_v1, 1 },
            { id_path_v1_api,       map_v1_api, 1 }
        };

        const UriPathMap* found = match(map, 3, "api", 0);

        REQUIRE(found != NULLPTR);
        REQUIRE(found->second == 1);

        // layer2 arrays are the size of a pointer, so we don't
        // need to go to lengths to shrink the size for UriPathMatcher2
        auto _map = estd::layer2::make_array(map);

        REQUIRE(sizeof(_map) == sizeof(void*));

        //estd::layer3::array<const UriPathMap> __map(map);
        // FIX: does not invoke move constructor like it should
        UriPathMatcher2<decltype(_map)> matcher(std::move(map));

        // TODO: Should be able to make a global vs instance version of UriPathMatcher2,
        // but would mean a non-stack version of 'map'

        // NOTE: this particular class doesn't modify matcher state
        // but it's confusing which ones do and which ones don't.  more fuel
        // for the fire of making a distinct stateful variant of the matcher class
        estd::optional<int> result = matcher.find("v1", MCCOAP_URIPATH_NONE);

        REQUIRE(result.has_value());
        REQUIRE(result.value() == id_path_v1);

        result = matcher.find("v1");

        REQUIRE(result);
        REQUIRE(*result == id_path_v1);

        result = matcher.find("api");

        REQUIRE(result);
        REQUIRE(*result == id_path_v1_api);

        result = matcher.find("power");

        REQUIRE(result);
        REQUIRE(*result == id_path_v1_api_power);

        struct sax_responder
        {
            int last_parent;
            int last_pos;
            int indent;

            sax_responder() :
                last_pos(MCCOAP_URIPATH_NONE),
                last_parent(MCCOAP_URIPATH_NONE),
                indent(0)
                {}

            void on_notify(const known_uri_event& e)
            {
                std::cout << "Known path: ";
                //std::cout << std::setfill(' ') << std::setw(indent);
                std::cout << std::setw(10) << e.path_map.first;
                std::cout << ", parent=" << e.path_map.third << std::endl;

                // second = current node id
                int current_id = e.path_map.second;
                int parent_id = e.path_map.third;
                last_pos = e.path_map.second;
                last_parent = parent_id;
            }
        };

        CoREData coredata[] =
        {
            { id_path_v1_api_power, "star-power", "switch" },
            { id_path_v1_api_test, "star-power", "sensor" },
            { id_path_v2_api, "dummy-path", "noop" }
        };

        //auto _coredata = estd::layer2::make_array(coredata);

        // foundational/poc for emitter of CoRE for /.well-known/core responder
        struct core_observer
        {
            embr::void_subject link_attribute_evaluator;

            typedef estd::experimental::ostringstream<256> ostream_type;

            // in non-proof of concept, this won't be inline with the observer
            ostream_type buf;

            // all uripath nodes which actually have CoRE data associated
            // with them
            // TODO: A map would be better instead of a layer3 array
            estd::layer3::array<CoREData, uint8_t> coredata;

            // breadcrumb tracker for hierarchical navigation of nodes
            estd::layer1::stack<const UriPathMap*, 10> parents;

            core_observer(estd::layer3::array<CoREData, uint8_t> coredata) :
                coredata(coredata) {}

            // NOTE: doing this via notify but perhaps could easily do a more straight
            // ahead sequential version
            void on_notify(const known_uri_event& e)
            {
                // 'top' aka the back/last/current parent

                // if the last parent id we are looking at doesn't match incoming
                // observed one, back it out until it does match the observed one
                while(!parents.empty() && parents.top()->second != e.parent_id())
                    parents.pop();

                // when we arrive here, we're either positioned in 'parents' at the
                // matching parent OR it's empty
                parents.push(&e.path_map);

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
                    buf << '<';

                    // in atypical queue/stack usage we push to the back but evaluate (but not
                    // pop) the front/bottom.  This way we can tack on more and more uri's to reflect
                    // our hierarchy level, then pop them off as we leave
                    // NOTE: our layer1::stack can do this but the default stack cannot
                    for(const auto& parent_node : parents)
                        buf << '/' << parent_node->first;

                    buf << '>' << *result;

                    // tack on other link attributes
                    node_core_event<ostream_type> _e(result->node, buf);
                    link_attribute_evaluator.notify(_e);

                    auto link_attribute_evaluator2 = embr::layer1::make_subject(title_tacker());
                    link_attribute_evaluator2.notify(_e);

                    // TODO: We'll need to intelligently spit out a comma
                    // and NOT endl unless we specifically are in a debug mode
                    buf << estd::endl;
                }
            }
        };

        sax_responder observer;
        // TODO: fixup layer3::array so that it can copy between different size_t
        // variations of itself (especially upcasting to higher precision)
        core_observer observer2(estd::layer3::make_array(coredata));
        // this will make a subject with a reference to 'observer'
        auto subject = embr::layer1::make_subject(observer, observer2);
        matcher.notify(subject);

        const auto& underlying_string = observer2.buf.rdbuf()->str();

        std::cout << underlying_string;
        //REQUIRE(underlying_string == "v1\napi\npower\n");

        // match up incoming uri-path chunk
        // (in production we'd be tracking where we left off, so 'map_root'
        // wouldn't be hardcoded)
        int id = match(map_root, 1, "v1");

        if(id >= 0)
        {
            auto hit = match(relations, sizeof(relations), id);

            REQUIRE(hit);
            REQUIRE(hit->second == map_v1);
            //auto hit = std::find(relations, &relations[2], id);

            //REQUIRE(hit->second == map_v1);
        }

        struct test_factory
        {
            int key;

            test_factory(int key) : key(key) {}

            bool can_create(int key)
            {
                return this->key == key;
            }

            int create(int key)
            {
                return 5;
            }
        };

        //auto t = estd::make_tuple(1, 2, 3, 4);
        auto t = estd::make_tuple(test_factory(2), test_factory(1));
        moducom::coap::experimental::FactoryAggregator<int, decltype (t)&> fa(t);

        fa.create(1);

        SECTION("decoder interaction")
        {
            // NOTE: layer2 not what I expected (remembered).  layer2 for netbuf is preallocated
            // memory with a variable size parameter
            //embr::mem::layer2::NetBuf<sizeof(buffer_plausible)> netbuf(buffer_plausible);
            embr::mem::layer1::NetBuf<sizeof(buffer_plausible)> netbuf;

            memcpy(netbuf.data(), buffer_plausible, sizeof(buffer_plausible));

            typedef decltype(netbuf) netbuf_type;
            typedef decltype(netbuf)& netbuf_ref_type;

            // FIX: these fail still.  thought I had repaired it
            /*
            REQUIRE(NetBufDecoder<netbuf_type>::has_data_method<netbuf_type>::value);
            REQUIRE(NetBufDecoder<netbuf_ref_type>::has_data_method<
                    typename estd::remove_reference<netbuf_ref_type&>::type>::value);
            */
            //embr::s
            //NetBufDecoder<decltype(netbuf)&> decoder(netbuf);
        }
    }
}
