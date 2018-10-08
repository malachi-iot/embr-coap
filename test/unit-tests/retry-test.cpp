#include <catch.hpp>

#include "exp/misc.h"
#include <embr/datapump.hpp>
#include <embr/netbuf-dynamic.h>
#include <exp/retry.h>

#include <exp/message-observer.h>
#include "test-observer.h"

#include "exp/prototype/observer-idea1.h"

#include "coap/decoder/subject.hpp"
#if __cplusplus >= 201103L
#include "platform/generic/malloc_netbuf.h"
#endif

#include "test-data.h"

struct fake_time_traits
{
    typedef uint16_t time_t;

    static time_t m_now;

    static time_t now() { return m_now; }
};

struct UnitTestRetryRandPolicy
{
    static int rand(int lower, int upper) { return 2500; }
};

fake_time_traits::time_t fake_time_traits::m_now = 2000;

//
//#define USE_EMBR_NETBUF

TEST_CASE("retry logic")
{
    typedef uint32_t addr_t;
#ifdef USE_EMBR_NETBUF
    typedef embr::mem::experimental::NetBufDynamic<> netbuf_t;
#else
    typedef NetBufDynamicExperimental netbuf_t;
#endif
    typedef embr::TransportDescriptor<netbuf_t, addr_t> transport_descriptor_t;

    SECTION("Ensure ack test data is right")
    {
        const Header* h = reinterpret_cast<const Header*>(buffer_ack);

        REQUIRE(h->type() == Header::Acknowledgement);
    }
    SECTION("retry (raw)")
    {
        Retry<netbuf_t, addr_t> retry;
    }
    // datapump being externalized as much as possible from retry code
    SECTION("retry")
    {
        typedef Retry<netbuf_t, addr_t, RetryPolicy<fake_time_traits, UnitTestRetryRandPolicy> > retry_t;
        typedef embr::DataPump<transport_descriptor_t> datapump_t;
        addr_t fakeaddr;
        netbuf_t netbuf;
#ifdef USE_EMBR_NETBUF
        // like a PBUF we need to preallocate some memory and usually we can't be 100%
        // sure how much we're gonna need
        netbuf.expand(128, false);
        auto netbuf_data = [&]() { return netbuf.data(); };
#else
        auto netbuf_data = [&]() { return netbuf.unprocessed(); };
#endif

        memcpy(netbuf_data(), buffer_16bit_delta, sizeof(buffer_16bit_delta));
        Header *header = new (netbuf_data()) Header;;

        header->type(Header::Confirmable);
        REQUIRE(header->message_id() == 0x123);

#ifdef USE_EMBR_NETBUF
        #error NOT READY YET - need a shrink operation
#else
        netbuf.advance(sizeof(buffer_16bit_delta));
#endif

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

#ifdef FEATURE_MCCOAP_RETRY_INLINE
            retry.enqueue(std::move(netbuf), fakeaddr);
#else
            retry.enqueue(netbuf, fakeaddr);
#endif

            // simulate queue to send.  assumes (correctly so, always)
            // that this is a CON message.  This is our first (non retry)
            // send
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.enqueue_out(std::move(netbuf), fakeaddr);
#else
            datapump.enqueue_out(netbuf, fakeaddr);
#endif

            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                REQUIRE(datapump_item.addr() == fakeaddr);
#ifdef UNUSED
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
#endif
            }
            // simulate transport send
            datapump.transport_pop();

            // Using 2000 because enqueue, for unit tests, is hardwired to that start
            // time
            // first retry should occur at 2000 + 2500 = 4500, so poke it at 4600
            // we expect this will enqueue things again
            // this issues retransmit #1
            retry.service_retry(4600, datapump);

            REQUIRE(!datapump.transport_empty());
            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                REQUIRE(datapump_item.addr() == fakeaddr);
#ifdef UNUSED
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
#endif
            }
            datapump.transport_pop(); // make believe we sent it somewhere

            REQUIRE(datapump.transport_empty());
\
            // second retry should occur at 4600 + 5000 = 9600
            // this issues retransmit #2
            retry.service_retry(9600, datapump);

            // simulate transport send
            datapump.transport_pop();
            // nothing left in outgoing queue after that send
            REQUIRE(datapump.transport_empty());

            // third retry should occur at 9600 + 10000 = 19600
            // this issues retransmit #3, final retransmit
            retry.service_retry(19600, datapump);

            // ensure retry did queue a message
            REQUIRE(!datapump.transport_empty());
            {
                datapump_t::Item& datapump_item = datapump.transport_front();
#ifdef UNUSED
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(!retain); // final transmit does NOT retain netbuf
#endif
            }
            // simulate transport send
            datapump.transport_pop();
            // nothing left in outgoing queue after that send
            REQUIRE(datapump.transport_empty());

            // should have nothing left to resend, we ran out of tries
            retry.service_retry(25000, datapump);

            REQUIRE(datapump.transport_empty());
        }
        SECTION("retry.service - ack")
        {
            datapump_t datapump;

#ifdef FEATURE_MCCOAP_RETRY_INLINE
            retry.enqueue(std::move(netbuf), fakeaddr);
#else
            retry.enqueue(netbuf, fakeaddr);
#endif

            //REQUIRE(item.mid() == 0x123);

            // simulate queue to send.  assumes (correctly so, always)
            // that this is a CON message
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.enqueue_out(std::move(netbuf), fakeaddr);
#else
            datapump.enqueue_out(netbuf, fakeaddr);
#endif

            {
                datapump_t::Item& datapump_item = datapump.transport_front();
                REQUIRE(datapump_item.addr() == fakeaddr);
#ifdef UNUSED
                bool retain = datapump_item.on_message_transmitted(); // pretend we sent it and invoke observer
                REQUIRE(retain); // expect we are retaining netbuf
#endif
            }
            // simulate transport send
            datapump.transport_pop();

            REQUIRE(datapump.dequeue_empty());

            netbuf_t simulated_ack;

#ifdef USE_EMBR_NETBUF
#error NOT READY YET - need a shrink operation
            simulated_ack.expand(128, false);
            memcpy(simulated_ack.data(), buffer_ack, sizeof(buffer_ack));
            //simulated_ack.shrink(sizeof(buffer_ack));
#else
            memcpy(simulated_ack.unprocessed(), buffer_ack, sizeof(buffer_ack));
            simulated_ack.advance(sizeof(buffer_ack));
#endif

#ifdef FEATURE_EMBR_DATAPUMP_INLINE
            datapump.transport_in(std::move(simulated_ack), fakeaddr);
#else
            datapump.transport_in(simulated_ack, fakeaddr);
#endif

            REQUIRE(!datapump.dequeue_empty());

            retry.service_ack(datapump);
        }
    }
}
