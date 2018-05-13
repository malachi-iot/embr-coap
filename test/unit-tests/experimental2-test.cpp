#include <catch.hpp>

#include <exp/datapump.hpp>
#include <exp/retry.h>

#include <exp/message-observer.h>
#include "test-observer.h"

#include "exp/prototype/observer-idea1.h"

using namespace moducom::coap::experimental;

TEST_CASE("experimental 2 tests")
{
    SECTION("A")
    {
        int fakenetbuf = 5;
        uint8_t fakeaddr[4];

        Retry<int> retry;

        retry.enqueue(fakeaddr, fakenetbuf);

        // Not yet, need newer estdlib first with cleaned up iterators
        Retry<int>::Item* test = retry.front();
    }
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
        using namespace ::experimental::prototype;

        DecoderObserver test_target1(0);
        DecoderObserver test_target2(0);

        URI uri[] =
        {
            URI("target1", test_target1),
            URI("target2", test_target2)
        };

        DecoderObserver d(uri);
    }
}
