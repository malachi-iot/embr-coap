//
// Created by malachi on 12/27/17.
//

#include <catch.hpp>

#include "../coap-dispatcher.h"
#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;
using namespace moducom::pipeline;

// Specialized prefix checker which does not expect s to be
// null terminated, but does expect prefix to be null terminated
template <typename TChar>
bool starts_with(const TChar* s, int slen, const char* prefix)
{
    while(*prefix && slen--)
    {
        if(*prefix++ != *s++) return false;
    }

    return true;
}


bool starts_with(MemoryChunk::readonly_t chunk, const char* prefix)
{
    return starts_with(chunk.data(), chunk.length(), prefix);
}


class UriPathDispatcherHandlerBaseBase : public DispatcherHandlerBase
{
protected:
    const char* prefix;

    UriPathDispatcherHandlerBaseBase(const char* prefix) : prefix(prefix)
    {}

    void on_header(Header header) OVERRIDE
    {
        if(!header.is_request()) interested(Never);
    }
};

template <class TObserver>
class UriPathDispatcherHandlerBase : public UriPathDispatcherHandlerBaseBase
{
protected:
    typedef UriPathDispatcherHandlerBaseBase base_t;
    TObserver& observer;

public:
    UriPathDispatcherHandlerBase(const char* prefix, TObserver& observer)
            : base_t(prefix),
              observer(observer)
    {

    }

    void on_option(number_t number, uint16_t length) OVERRIDE
    {
        if(is_always_interested())
        {
            observer.on_option(number, length);
            return;
        }
    }

    void on_option(number_t number,
                   const MemoryChunk::readonly_t& option_value_part,
                   bool last_chunk) OVERRIDE
    {
        // If we're always interested, then we've gone into pass thru mode
        if(is_always_interested())
        {
            observer.on_option(number, option_value_part, last_chunk);
            return;
        }

        switch(number)
        {
            case Option::UriPath:
                // FIX: I think we don't need this, because path is already
                // chunked out so 99% of the time we'll want to compare the
                // whole string not just the prefix - but I suppose comparing
                // the as a prefix doesn't hurt us
                if(starts_with(option_value_part, prefix))
                    interested(Always);
                else
                    // If we don't get what we want right away, then we are
                    // no longer interested (more prefix style behavior)
                    interested(Never);

                // TODO: We'll want to not only indicate interest but also
                // have a subordinate dispatcher to act on further observations
                break;
        }
    }

    void on_payload(const MemoryChunk::readonly_t& payload_part, bool last_chunk) OVERRIDE
    {
        ASSERT_WARN(true, is_always_interested(), "Should only arrive here if interested");

        observer.on_payload(payload_part, last_chunk);
    }
};

class UriPathDispatcherHandler :
        public UriPathDispatcherHandlerBase<IOptionAndPayloadObserver>
{
    typedef UriPathDispatcherHandlerBase<IOptionAndPayloadObserver> base_t;
public:
    UriPathDispatcherHandler(const char* prefix, IOptionAndPayloadObserver& observer)
            : base_t(prefix, observer)
    {

    }
};

class TestDispatcherHandler : public DispatcherHandlerBase
{
    int option_test_number;

public:
    TestDispatcherHandler(InterestedEnum i = Always) :
            DispatcherHandlerBase(i),
            option_test_number(0) {}

    virtual void on_header(Header header) OVERRIDE
    {
        REQUIRE(header.is_request());
    }

    virtual void on_token(const MemoryChunk::readonly_t& token_part, bool last_chunk) OVERRIDE
    {
        FAIL("Should not reach here");
    }

    virtual void on_option(number_t number, uint16_t length) OVERRIDE
    {
        switch(option_test_number)
        {
            case 0:
                REQUIRE(number == 270);
                REQUIRE(length == 1);
                break;

            case 1:
                REQUIRE(number == 271);
                REQUIRE(length == 2);
                break;
        }
    }

    virtual void on_option(number_t number, const MemoryChunk::readonly_t& option_value_part, bool last_chunk) OVERRIDE
    {
        switch(option_test_number++)
        {
            case 0:
                REQUIRE(option_value_part[0] == 3);
                REQUIRE(option_value_part.length() == 1);
                break;

            case 1:
                REQUIRE(option_value_part[0] == 4);
                REQUIRE(option_value_part[1] == 5);
                REQUIRE(option_value_part.length() == 2);
                break;
        }
    }

    virtual void on_payload(const MemoryChunk::readonly_t& payload_part, bool last_chunk) OVERRIDE
    {
        REQUIRE(payload_part[0] == buffer_16bit_delta[12]);
        REQUIRE(payload_part[payload_part.length()] == buffer_16bit_delta[12 + payload_part.length()]);
    }
};

extern dispatcher_handler_factory_fn test_sub_factories[];

IDispatcherHandler* test_factory1(MemoryChunk chunk)
{
    return new (chunk.data()) TestDispatcherHandler(IDispatcherHandler::Never);
}


// TODO: Fix this name
template <dispatcher_handler_factory_fn* factories, int count, const char* uri_path>
IDispatcherHandler* uri_helper(MemoryChunk chunk)
{
    MemoryChunk& uri_handler_chunk = chunk;
    // semi-objstack behavior
    MemoryChunk sub_handler_chunk = chunk.remainder(sizeof(UriPathDispatcherHandler));
    MemoryChunk sub_handler_inner_chunk = sub_handler_chunk.remainder(sizeof(FactoryDispatcherHandler));

    FactoryDispatcherHandler* fdh = new (sub_handler_chunk.data()) FactoryDispatcherHandler(
            sub_handler_inner_chunk,
            factories, count);

    return new (uri_handler_chunk.data()) UriPathDispatcherHandler(uri_path, *fdh);
}


// Does not work because compiler struggles to cast back down to IMessageObserver
// for some reason
template <const char* uri_path, IMessageObserver* observer>
IDispatcherHandler* uri_helper3(MemoryChunk chunk)
{
    return new (chunk.data()) UriPathDispatcherHandler(uri_path, *observer);
}


// Creates a unique static TMessageObserver associated with this uri_path
template <const char* uri_path, class TMessageObserver>
IDispatcherHandler* uri_helper4(MemoryChunk chunk)
{
    static TMessageObserver observer;

    return new (chunk.data()) UriPathDispatcherHandler(uri_path, observer);
}


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
IDispatcherHandler* uri_helper(MemoryChunk chunk)
{

} */

IDispatcherHandler* test_factory2(MemoryChunk chunk)
{
    MemoryChunk& uri_handler_chunk = chunk;
    MemoryChunk v1_handler_chunk = chunk.remainder(sizeof(UriPathDispatcherHandler));
    MemoryChunk v1_handler_inner_chunk = v1_handler_chunk.remainder(sizeof(FactoryDispatcherHandler));

    FactoryDispatcherHandler* fdh = new (v1_handler_chunk.data()) FactoryDispatcherHandler(
            v1_handler_inner_chunk,
            test_sub_factories, 1);

    // TODO: will need some way to invoke fdh destructor
    //return new (uri_handler_chunk.data()) UriPathDispatcherHandler("v1", *fdh);
    // TEST instead of v1 to work with our existing test data
    return new (uri_handler_chunk.data()) UriPathDispatcherHandler("OLD_TEST", *fdh);
}

extern CONSTEXPR char TEST_FACTORY4_NAME[] = "TEST";


template <typename T, int size>
CONSTEXPR dispatcher_handler_factory_fn uri_helper_helper(T a [size], const char* name)
{
    return &uri_helper<a, size, TEST_FACTORY4_NAME>;
};


template <class TArray>
dispatcher_handler_factory_fn uri_helper2(CONSTEXPR TArray& array)
{
    typedef array_helper<TArray> array_helper_t;

    CONSTEXPR int count = _array_helper_count(array);
    const void* contents = _array_helper_contents(array);

    // really needs real constexpr from C++11 to work
    //return &uri_helper<array, count, name>;

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
    test_factory1,
    test_factory2,
    uri_helper<test_sub_factories, 1, TEST_FACTORY4_NAME>,
    //uri_helper_helper(test_sub_factories, "test")
    //uri_helper_helper2<TEST_FACTORY4_NAME>(test_sub_factories)
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


IDispatcherHandler* helper1(MemoryChunk chunk)
{
    // compiler struggles to cast pos_handler to IMessageObserver ...
    // for some reason
    //return uri_helper3<POS_HANDLER_URI, &pos_handler>(chunk);
}

dispatcher_handler_factory_fn test_sub_factories[] =
{
        uri_helper4<POS_HANDLER_URI, TestDispatcherHandler2>
        //helper1
//    uri_helper3<POS_HANDLER_URI, _pos_handler>
        //uri_helper3<POS_HANDLER_URI, _pos_handler>
};

TEST_CASE("CoAP dispatcher tests", "[coap-dispatcher]")
{
    SECTION("Test 1")
    {
        MemoryChunk chunk(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        TestDispatcherHandler dispatcherHandler;
        Dispatcher dispatcher;
        //layer3::MemoryChunk<128> chunk;

        dispatcher.head(&dispatcherHandler);
        dispatcher.dispatch(chunk);


    }
    SECTION("Factory")
    {
        MemoryChunk chunk(buffer_plausible, sizeof(buffer_plausible));

        // in-place new holder
        layer3::MemoryChunk<128> dispatcherBuffer;

        FactoryDispatcherHandler fdh(dispatcherBuffer, test_factories, 3);
        Dispatcher dispatcher;

        // doesn't fully test new UriPath handler because TestDispatcherHandler
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
}