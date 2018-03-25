//
// Created by malachi on 12/27/17.
//

#include <catch.hpp>

#include "../coap-dispatcher.h"
#include "../coap-uripath-dispatcher.h"
#include "test-data.h"
#include "test-observer.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;



extern dispatcher_handler_factory_fn test_sub_factories[];

IDispatcherHandler* context_handler_factory(FactoryDispatcherHandlerContext& ctx)
{
    IncomingContext& context = ctx.incoming_context;

#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(context);
#else
    static moducom::dynamic::PoolBase<moducom::coap::layer2::Token, 8> token_pool;
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(context, token_pool);
#endif
}


IDispatcherHandler* test_factory1(FactoryDispatcherHandlerContext& ctx)
{
    return new (ctx.handler_memory.data()) Buffer16BitDeltaObserver(IsInterestedBase::Never);
}


/*
// Does not work because compiler struggles to cast back down to IMessageObserver
// for some reason
template <const char* uri_path, IMessageObserver* observer>
IDispatcherHandler* uri_helper3(MemoryChunk chunk)
{
    return new (chunk.data()) UriPathDispatcherHandler(uri_path, *observer);
}
*/

template <typename T>
struct array_helper
{
    static CONSTEXPR int count() { return 0; }
};

template <typename T, ptrdiff_t N>
struct array_helper<T[N]>
{
    typedef T element_t;
    static CONSTEXPR ptrdiff_t count() { return N; }
};


/*
template <class TArray>
int _array_helper_count(TArray array)
{
    array_helper<TArray, > array_descriptor;

    return array_helper<TArray>::count();
} */

template <typename T, int size>
CONSTEXPR int _array_helper_count(T (&) [size]) { return size; }

template <typename T, int size>
CONSTEXPR T* _array_helper_contents(T (&t) [size]) { return t; }


/*
template <dispatcher_handler_factory_fn factories[N], int N>
IDispatcherHandler* uri_plus_factory_dispatcher(MemoryChunk chunk)
{

} */

IDispatcherHandler* test_factory2(FactoryDispatcherHandlerContext& ctx)
{
    MemoryChunk& uri_handler_chunk = ctx.handler_memory;
    MemoryChunk v1_handler_chunk = ctx.handler_memory.remainder(sizeof(SingleUriPathObserver));
    MemoryChunk v1_handler_inner_chunk = v1_handler_chunk.remainder(sizeof(FactoryDispatcherHandler));

    FactoryDispatcherHandler* fdh = new (v1_handler_chunk.data()) FactoryDispatcherHandler(
            v1_handler_inner_chunk,
            ctx.incoming_context,
            test_sub_factories, 1);

    // TODO: will need some way to invoke fdh destructor
    return new (uri_handler_chunk.data()) SingleUriPathObserver("v1", *fdh);
}

extern CONSTEXPR char STR_TEST[] = "TEST";


/*
template <typename T, int size>
CONSTEXPR dispatcher_handler_factory_fn uri_helper_helper(T a [size], const char* name)
{
    return &uri_plus_factory_dispatcher<TEST_FACTORY4_NAME, a, size>;
}; */


template <class TArray>
dispatcher_handler_factory_fn uri_helper2(const TArray& array)
{
    typedef array_helper<TArray> array_helper_t;

    int count = _array_helper_count(array);
    const void* contents = _array_helper_contents(array);

    // really needs real constexpr from C++11 to work
    //return &uri_plus_factory_dispatcher<array, count, name>;

    /*
    MemoryChunk& uri_handler_chunk = chunk;
    MemoryChunk sub_handler_chunk = chunk.remainder(sizeof(UriPathDispatcherHandler));
    MemoryChunk sub_handler_inner_chunk = sub_handler_chunk.remainder(sizeof(FactoryDispatcherHandler));

    FactoryDispatcherHandler* fdh = new (sub_handler_chunk.data()) FactoryDispatcherHandler(
            sub_handler_inner_chunk,
            array, count);

    return new (uri_handler_chunk.data()) UriPathDispatcherHandler(uri_path, *fdh); */
};


dispatcher_handler_factory_fn test_factories[] =
{
    context_handler_factory,
    test_factory1,
    test_factory2,
    uri_plus_factory_dispatcher<STR_TEST, test_sub_factories, 1>
};


class TestDispatcherHandler2 : public DispatcherHandlerBase
{
public:
    void on_payload(const MemoryChunk::readonly_t& payload_part,
                    bool last_chunk) OVERRIDE
    {
        REQUIRE(payload_part[0] == 0x10);
        REQUIRE(payload_part[1] == 0x11);
        REQUIRE(payload_part[2] == 0x12);
        REQUIRE(payload_part.length() == 7);
    }
};


extern CONSTEXPR char POS_HANDLER_URI[] = "POS";

extern TestDispatcherHandler2 pos_handler;
TestDispatcherHandler2 pos_handler;


dispatcher_handler_factory_fn test_sub_factories[] =
{
    uri_plus_observer_dispatcher<POS_HANDLER_URI, TestDispatcherHandler2>
};

TEST_CASE("CoAP dispatcher tests", "[coap-dispatcher]")
{
    SECTION("Test 1")
    {
        MemoryChunk chunk(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        Buffer16BitDeltaObserver dispatcherHandler;
        Dispatcher dispatcher;
        //layer3::MemoryChunk<128> chunk;

        dispatcher.head(&dispatcherHandler);
        dispatcher.dispatch(chunk);


        // Dummy test just so we can set a breakpoint here
        REQUIRE(chunk.length() == sizeof(buffer_16bit_delta));
    }
    SECTION("Factory")
    {
        MemoryChunk chunk(buffer_plausible, sizeof(buffer_plausible));

        // in-place new holder
        // FIX: OK all those virtual function tables seem to be bloating
        // our handlers way, way up...
        layer3::MemoryChunk<512> dispatcherBuffer;
        IncomingContext context;

        FactoryDispatcherHandler fdh(dispatcherBuffer, context, test_factories);
        Dispatcher dispatcher;

        // doesn't fully test new UriPath handler because Buffer16BitDeltaObserver
        // is not stateless (and shouldn't be expected to be) but because of that
        // setting it to "Currently" makes it unable to test its own options properly
        dispatcher.head(&fdh);
        dispatcher.dispatch(chunk);
    }
    SECTION("Array experimentation")
    {
        int array[] = { 1, 2, 3 };

        int count = _array_helper_count(array);

        REQUIRE(count == 3);
    }
    SECTION("Size ofs")
    {
        int size1 = sizeof(IMessageObserver);
#ifdef FEATURE_DISCRETE_OBSERVERS
        int size2 = sizeof(IOptionAndPayloadObserver);
        int size3 = sizeof(IOptionObserver);
#endif
        int size4 = sizeof(IDispatcherHandler);
        int size5 = sizeof(IIsInterested);
        int size6 = sizeof(DispatcherHandlerBase);
        int size7 = sizeof(SingleUriPathObserver);
        int size8 = sizeof(FactoryDispatcherHandler);
    }
}
