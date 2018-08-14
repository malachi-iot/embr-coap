#include <catch.hpp>

//#define FEATURE_ESTD_EXPLICIT_OBSERVER

#include <platform/generic/datapump-messageobserver.hpp>
#include <embr/datapump.hpp>
#include "coap/decoder/subject.hpp"

#include "test-data.h"
#include "test-observer.h"

#include <estd/exp/observer.h>

//#define putchar(c) put_value(c)

using namespace moducom::coap;
using namespace embr;

typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
typedef uint32_t addr_t;
typedef TransportDescriptor<netbuf_t, addr_t> transport_descriptor_t;
typedef DataPump<transport_descriptor_t> datapump_t;
typedef datapump_t::Item item_t;

struct DatapumpObserver
{
    // thought const& would be necessary because of lack of copy constructors, but nope
    static void on_notify(moducom::coap::experimental::event::ReceiveDequeued<item_t> e)
    {
        REQUIRE(e.item.addr() == 0);
    }
};

TEST_CASE("Data pump tests", "[datapump]")
{
    SECTION("Datapump")
    {
        //typedef moducom::io::experimental::layer2::NetBufMemory<512> netbuf_t;
        //typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
        //typedef uint32_t addr_t;

        // will only work if I make it <const char, but then
        // s.copy won't work since it wants to output to value_type*
        // which will be const char*
        //const estd::layer2::basic_string<char, 4> s = "Hi2u";

        datapump_t datapump;
        addr_t addr;

        netbuf_t netbuf;

        moducom::io::experimental::NetBufWriter<netbuf_t&> writer(netbuf);

        //writer.write(s);

        writer.write("hi2u", 4);

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        datapump.enqueue_out(std::move(writer.netbuf()), addr);
#else
        datapump.enqueue_out(writer.netbuf(), addr);
#endif

        datapump_t::Item& item = datapump.transport_front();
        netbuf_t* to_transport = item.netbuf();
        addr = item.addr();

        REQUIRE(to_transport != NULLPTR);

        const uint8_t* p = to_transport->processed();

        REQUIRE(p[0] == 'h');

    }
    SECTION("API collision test")
    {
        netbuf_t netbuf;

        moducom::io::experimental::NetBufWriter<netbuf_t&> writer(netbuf);

        writer.putchar(0x77);

        REQUIRE(netbuf.processed()[0] == 0x77);
    }
    SECTION("full synthetic request")
    {
        // FIX: Need to resolve end() behavior.  It's more efficient for end() to represent
        // that we are *ON* the end marker, but it's smoother code to have end() represent
        // -just past- the end, ala std iterator behaviors

        // set up netbuf-writer, datapump
        //typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
        //typedef uint32_t addr_t;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        moducom::io::experimental::NetBufWriter<netbuf_t> writer;
#else
        // FIX: doing this because of floating 'delete' within process_messageobserver
        // clearly not ideal
        netbuf_t* netbuf = new netbuf_t;

        moducom::io::experimental::NetBufWriter<netbuf_t&> writer(*netbuf);
#endif
        datapump_t datapump;

        // push synthetic incoming coap request data into netbuf via writer
        writer.write(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        // Now take that synthetic netbuf data and simulates a transport input
        // on datapump
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        netbuf_t netbuf_copy = writer.netbuf();
        datapump.transport_in(std::move(writer.netbuf()), 0);
#else
        datapump.transport_in(writer.netbuf(), 0);
#endif

        // set up message subject+observer
        IncomingContext<addr_t> test_ctx;
        DecoderSubjectBase<moducom::coap::experimental::ContextDispatcherHandler<IncomingContext<addr_t> > > test(test_ctx);

        REQUIRE(!test_ctx.have_header());


        // now service the datapump with the 'test' DecoderSubject pushing out
        // to the message observer
        process_messageobserver(datapump, test);

        // Assert that ContextDispatcherHandler indeed parsed the incoming message
        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().message_id() == 0x0123);

#ifdef FEATURE_CPP_DECLTYPE
        // Requeue same synthetic data again for another test
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        datapump.transport_in(std::move(netbuf_copy), 0);
#else
        datapump.transport_in(writer.netbuf(), 0);
#endif


        SECTION("Experimental datapump dequeuing test")
        {
            estd::experimental::internal::stateless_subject<DatapumpObserver> s;

            process_messageobserver2(datapump, s);
        }
#endif
    }
}
