#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/observer.h>
#include <embr/netbuf-static.h>

#include <exp/retry.h>

//#include <exp/message-observer.h>
#include "test-observer.h"

#include "exp/prototype/observer-idea1.h"

#include "coap/decoder/subject.hpp"
#if __cplusplus >= 201103L
//#include "obsolete/platform/generic/malloc_netbuf.h"
#endif

#include "exp/events.h"
#include <exp/uripath-repeater.h>

//#include <estd/map.h>
#include <estd/sstream.h>
#include <estd/internal/ostream_basic_string.hpp>

#include "test-data.h"
#include "test-uri-data.h"

using namespace embr::coap;
using namespace embr::coap::experimental;

//typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
//typedef moducom::pipeline::MemoryChunk chunk_t;



TEST_CASE("experimental 2 tests")
{
    typedef uint32_t addr_t;


    /*
     * Disabled as part of mc-mem removal
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
        //embr::coap::layer1::Token token = ;
        uint8_t raw_token[] = { 0x45, 0x67 };

        test_header.token_length(sizeof(raw_token));
        test_header.message_id(0x0123);

        amo.on_header(test_header);
        //amo.on_token(raw_token); // not doing this only because Buffer16BitDeltaObserver hates it
        amo.on_option((embr::coap::Option::Numbers) 270, 1);
    }
#endif
     */
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
        embr::coap::experimental::UriPathRepeater<embr::void_subject> upr;
    }
    SECTION("factory test")
    {
        using namespace uri;

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
        internal::triad<int, UriPathMap2*, int> relations[]
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

        SECTION("main")
        {
        sax_responder observer;
        // TODO: fixup layer3::array so that it can copy between different size_t
        // variations of itself (especially upcasting to higher precision)
        core_observer<> observer2(estd::layer3::make_array(coredata));
        // this will make a subject with a reference to 'observer'
        auto subject = embr::layer1::make_subject(observer, observer2);
        matcher.notify(subject);

        const auto& underlying_string = observer2.out.rdbuf()->str();

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
        embr::coap::experimental::FactoryAggregator<int, decltype (t)&> fa(t);

        fa.create(1);
        }

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
        // TODO: Move this out of experimental area
        SECTION("internal UriPathMatcher (formerly UriPathMatcher3)")
        {
            embr::coap::internal::UriPathMatcher m(map);
            typedef const embr::coap::internal::UriPathMap* result_type;

            SECTION("Look in v2/api")
            {
                result_type v = m.find("v2");

                REQUIRE(v);
                REQUIRE(v->second == id_path_v2);

                v = m.find("api");

                REQUIRE(v);

                REQUIRE(v->second == id_path_v2_api);

                SECTION("Look in v2/api/nested/nested")
                {
                    result_type v = m.find("nested");

                    REQUIRE(v);
                    REQUIRE(v->second == id_path_v2_api_nested);

                    v = m.find("nested");

                    REQUIRE(v->second == id_path_v2_api_nested2);
                }
                SECTION("Look in v2/api/test")
                {
                    result_type v = m.find("test");

                    REQUIRE(v);
                    REQUIRE(v->second == id_path_v2_api_test);
                }
            }
            SECTION("look for v3")
            {
                result_type v = m.find("v3");

                REQUIRE(!v);
            }
        }
    }
    SECTION("v4 retry")
    {
        SECTION("factory")
        {
            typedef estd::span<const uint8_t> buffer_type;
            typedef DecoderFactory<buffer_type> factory;

            buffer_type b(buffer_simplest_request);

            auto d = factory::create(b);

            auto r = d.process_iterate_streambuf();
            r = d.process_iterate_streambuf();

            REQUIRE(d.state() == Decoder::HeaderDone);

            Header h = d.header_decoder();

            REQUIRE(h.message_id() == 0x123);
        }
    }
}
