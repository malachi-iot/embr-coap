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

class UriPathDispatcherHandler : public DispatcherHandlerBase
{
    const char* prefix;
    IMessageObserver& observer;

public:
    UriPathDispatcherHandler(const char* prefix, IMessageObserver& observer)
            : prefix(prefix),
              observer(observer)
    {

    }

    void on_header(Header header) OVERRIDE
    {
        if(!header.is_request()) interested(Never);
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


template <dispatcher_handler_factory_fn* factories, int count, const char* uri_path>
IDispatcherHandler* uri_helper(MemoryChunk chunk)
{
    MemoryChunk& uri_handler_chunk = chunk;
    MemoryChunk sub_handler_chunk = chunk.remainder(sizeof(UriPathDispatcherHandler));
    MemoryChunk sub_handler_inner_chunk = sub_handler_chunk.remainder(sizeof(FactoryDispatcherHandler));

    FactoryDispatcherHandler* fdh = new (sub_handler_chunk.data()) FactoryDispatcherHandler(
            sub_handler_inner_chunk,
            factories, count);

    return new (uri_handler_chunk.data()) UriPathDispatcherHandler(uri_path, *fdh);
}

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

dispatcher_handler_factory_fn test_factories[] =
{
    test_factory1,
    test_factory2,
    uri_helper<test_sub_factories, 1, TEST_FACTORY4_NAME>
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

IDispatcherHandler* test_sub_factory1(MemoryChunk chunk)
{
    static TestDispatcherHandler2 handler;

    // To be v1/path1
    //return new (chunk.data()) UriPathDispatcherHandler("path1", handler);
    // TEST/POS to work with our existing test data
    return new (chunk.data()) UriPathDispatcherHandler("POS", handler);
}

dispatcher_handler_factory_fn test_sub_factories[] =
{
    test_sub_factory1
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
}