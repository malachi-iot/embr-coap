#include <catch.hpp>

#include <exp/datapump.hpp>
#include <exp/retry.h>

#include <exp/message-observer.h>
#include "test-observer.h"

#include "exp/prototype/observer-idea1.h"

#include "coap/decoder/subject.hpp"
#include "platform/generic/malloc_netbuf.h"

#include "test-data.h"

using namespace moducom::coap;
using namespace moducom::coap::experimental;

typedef moducom::pipeline::MemoryChunk::readonly_t ro_chunk_t;
typedef moducom::pipeline::MemoryChunk chunk_t;

struct fake_time_traits
{
    typedef uint16_t time_t;

    static time_t m_now;

    static time_t now() { return m_now; }
};

fake_time_traits::time_t fake_time_traits::m_now = 2000;

TEST_CASE("experimental 2 tests")
{
    typedef uint32_t addr_t;

    SECTION("retry")
    {
        typedef NetBufDynamicExperimental netbuf_t;
        typedef Retry<netbuf_t, addr_t, fake_time_traits> retry_t;
        typedef DataPump<netbuf_t, addr_t> datapump_t;
        addr_t fakeaddr;
        netbuf_t netbuf;

        memcpy(netbuf.unprocessed(), buffer_16bit_delta, sizeof(buffer_16bit_delta));
        Header *header = new (netbuf.unprocessed()) Header(Header::Confirmable);

        netbuf.advance(sizeof(buffer_16bit_delta));

        retry_t retry;

        SECTION("retransmission low level logic")
        {

            // Not yet, need newer estdlib first with cleaned up iterators
            // commented our presently because of transition away from Metadata_Old
            //Retry<int, addr_t>::Item* test = retry.front();

            retry_t::Metadata metadata;

            metadata.initial_timeout_ms = 2500;
            metadata.retransmission_counter = 0;

            REQUIRE(metadata.delta() == 2500);

            metadata.retransmission_counter++;

            REQUIRE(metadata.delta() == 5000);

            metadata.retransmission_counter++;

            REQUIRE(metadata.delta() == 10000);
        }
        SECTION("retry.service - isolated case")
        {
            datapump_t datapump;

            retry_t::Item& item = retry.enqueue(netbuf, fakeaddr);

            // simulate queue to send.  assumes (correctly so, always)
            // that this is a CON message
            datapump.enqueue_out(netbuf, fakeaddr, &item);

            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                REQUIRE(datapump_item.addr() == fakeaddr);
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
            }
            // simulate transport send
            datapump.transport_pop();

            // first retry should occur at 2000 + 2500 = 4500, so poke it at 4600
            // we expect this will enqueue things again
            retry.service(4600, datapump);

            REQUIRE(!datapump.transport_empty());
            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                REQUIRE(datapump_item.addr() == fakeaddr);
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
            }
            datapump.transport_pop(); // make believe we sent it somewhere

            REQUIRE(datapump.transport_empty());
\
            // second retry should occur at 2000 + 5000 = 7000, so poke it at 7100
            retry.service(7100, datapump);

            // simulate transport send
            datapump.transport_pop();
            // nothing left in outgoing queue after that send
            REQUIRE(datapump.transport_empty());

            // third retry should occur at 2000 + 10000 = 12000, so poke it at 12100
            retry.service(12100, datapump);

            // ensure retry did queue a message
            REQUIRE(!datapump.transport_empty());
            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
            }
            // simulate transport send
            datapump.transport_pop();
            // nothing left in outgoing queue after that send
            REQUIRE(datapump.transport_empty());

            // fourth and final retry should occur at 2000 + 20000 = 22000, so poke it at 22100
            retry.service(22100, datapump);

            REQUIRE(!datapump.transport_empty());
            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(!retain); // expect we are NOT retaining netbuf (actually no observer should be here)
            }
            // simulate transport send
            datapump.transport_pop();
            // nothing left in outgoing queue after that send
            REQUIRE(datapump.transport_empty());

            // should have nothing left to resend, we ran out of tries
            retry.service(25000, datapump);

            REQUIRE(datapump.transport_empty());
        }
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
}
