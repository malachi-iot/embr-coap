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

#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
typedef moducom::pipeline::MemoryChunk chunk_t;


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
        constexpr int id_path_dummy = 3;

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
            { "power",  id_path_v1_api_power,   id_path_v1_api }
        };

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

        sax_responder observer;
        // this will make a subject with a reference to 'observer'
        auto subject = embr::layer1::make_subject(observer);
        matcher.notify(subject);

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
