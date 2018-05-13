//
// Created by malachi on 12/27/17.
//

#include <catch.hpp>

#include "coap-dispatcher.h"
#include "coap-uripath-dispatcher.h"
#include "test-data.h"
#include "test-observer.h"

#include "coap/decoder/subject.hpp"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;

extern dispatcher_handler_factory_fn test_sub_factories[];

template <class TRequestContext>
IDecoderObserver<TRequestContext>* context_handler_factory(TRequestContext& ctx)
{
#ifdef FEATURE_MCCOAP_INLINE_TOKEN
    return new (ctx) ContextDispatcherHandler<TRequestContext>(ctx);
#else
    static moducom::dynamic::PoolBase<moducom::coap::layer2::Token, 8> token_pool;
    return new (ctx.handler_memory.data()) ContextDispatcherHandler(ctx, token_pool);
#endif
}


template <class TRequestContext>
IDecoderObserver<TRequestContext>* test_buffer16bitdelta_observer(TRequestContext& ctx)
{
    return new (ctx) Buffer16BitDeltaObserver<TRequestContext>(IsInterestedBase::Never);
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
}


/*
template <dispatcher_handler_factory_fn factories[N], int N>
IDispatcherHandler* uri_plus_factory_dispatcher(MemoryChunk chunk)
{

} */

template <class TRequestContext>
IDecoderObserver<TRequestContext>* test_single_uri_observer(TRequestContext& ctx)
{
    // FIX: Clumsy, but should be effective for now; ensures order of allocation is correct
    //      so that later deallocation for objstack doesn't botch
    void* buffer1 = ctx.objstack.alloc(sizeof(SingleUriPathObserver<TRequestContext>));

    FactoryDispatcherHandler<TRequestContext>* fdh =
            new (ctx) FactoryDispatcherHandler<TRequestContext>(
            ctx,
            test_sub_factories, 1);

    return new (buffer1) SingleUriPathObserver<TRequestContext>("v1", *fdh);
}

extern CONSTEXPR char STR_TEST[] = "TEST";




dispatcher_handler_factory_fn test_factories[] =
{
    context_handler_factory,
    test_buffer16bitdelta_observer,
    test_single_uri_observer,
    uri_plus_factory_dispatcher<STR_TEST, test_sub_factories, 1>
};


template <class TRequestContext>
class TestDispatcherHandler2 : public DecoderObserverBase<TRequestContext>
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

//extern TestDispatcherHandler2<> pos_handler;
//TestDispatcherHandler2<> pos_handler;


dispatcher_handler_factory_fn test_sub_factories[] =
{
    uri_plus_observer_dispatcher<POS_HANDLER_URI, TestDispatcherHandler2<ObserverContext> >
};

TEST_CASE("CoAP dispatcher tests", "[coap-dispatcher]")
{
    SECTION("Dispatcher Factory")
    {
        MemoryChunk chunk(buffer_plausible, sizeof(buffer_plausible));

        moducom::pipeline::layer3::MemoryChunk<512> dispatcherBuffer;
        ObserverContext context(dispatcherBuffer);

        FactoryDispatcherHandler<ObserverContext> fdh(context, test_factories);

        DecoderSubjectBase<IDecoderObserver<ObserverContext> & > decoder_subject(fdh);

        decoder_subject.dispatch(chunk);
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
        int size4 = sizeof(IDecoderObserver<>);
        int size5 = sizeof(IIsInterested);
        int size6 = sizeof(DecoderObserverBase<>);
        int size7 = sizeof(SingleUriPathObserver<IncomingContext<uint32_t> >);
        int size8 = sizeof(FactoryDispatcherHandler<>);
    }
}
