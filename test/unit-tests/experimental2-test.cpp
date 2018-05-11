#include <catch.hpp>

#include <exp/datapump.hpp>
#include <exp/retry.h>

#include <exp/message-observer.h>
#include "test-observer.h"

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
    SECTION("AggregateMessageObserver")
    {
        moducom::pipeline::layer1::MemoryChunk<256> chunk;

        ObserverContext ctx(chunk);
        AggregateMessageObserver<ObserverContext,
                Buffer16BitDeltaObserver<ObserverContext> > amo(ctx);

        Header test_header(Header::Confirmable);

        test_header.message_id(0x0123);

        // FIX: almost there but not quite yet
        //amo.on_header(test_header);
    }
}
