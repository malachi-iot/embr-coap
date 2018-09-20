#include <catch.hpp>

#include "exp/misc.h"
#include <embr/datapump.hpp>
#include <embr/observer.h>
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

#include <estd/map.h>

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

        // best bet is to have this pre sorted alphabetically first, then by parent ID next
        // traditionally one would use a multimap here, since it's conceivable that different
        // tree uri nodes may have the same name (think v2/api)
        UriPathMap map[] =
        {
            { "api",    id_path_v1_api,         id_path_v1 },
            { "power",  id_path_v1_api_power,   id_path_v1_api },
            { "v1",     id_path_v1,             MCCOAP_URIPATH_NONE }
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

        UriPathMap* found = match(map, 3, "api", 0);

        REQUIRE(found != NULLPTR);
        REQUIRE(found->second == 1);

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
    }
}
