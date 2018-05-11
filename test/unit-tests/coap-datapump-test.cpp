#include <catch.hpp>

#include <platform/generic/datapump-messageobserver.hpp>
#include <exp/datapump.hpp>
#include "test-data.h"

using namespace moducom::coap;

typedef IncomingContext request_context_t;

TEST_CASE("Data pump tests", "[datapump]")
{
    SECTION("Datapump")
    {
        //typedef moducom::io::experimental::layer2::NetBufMemory<512> netbuf_t;
        typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
        typedef uint32_t addr_t;

        // will only work if I make it <const char, but then
        // s.copy won't work since it wants to output to value_type*
        // which will be const char*
        //const estd::layer2::basic_string<char, 4> s = "Hi2u";

        DataPump<netbuf_t, addr_t> datapump;
        addr_t addr;

        netbuf_t netbuf;

        moducom::io::experimental::NetBufWriter<netbuf_t&> writer(netbuf);

        //writer.write(s);

        writer.write("hi2u", 4);

        datapump.enqueue_out(writer.netbuf(), addr);

        netbuf_t* to_transport = datapump.transport_front(&addr);

        REQUIRE(to_transport != NULLPTR);

        const uint8_t* p = to_transport->processed();

        REQUIRE(p[0] == 'h');

    }
    SECTION("A")
    {
        // FIX: Need to resolve end() behavior.  It's more efficient for end() to represent
        // that we are *ON* the end marker, but it's smoother code to have end() represent
        // -just past- the end, ala std iterator behaviors

        // set up netbuf-writer, datapump
        typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
        typedef uint32_t addr_t;

#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
        moducom::io::experimental::NetBufWriter<netbuf_t> writer;
#else
        // FIX: doing this because of floating 'delete' within process_messageobserver
        // clearly not ideal
        netbuf_t* netbuf = new netbuf_t;

        moducom::io::experimental::NetBufWriter<netbuf_t&> writer(*netbuf);
#endif
        DataPump<netbuf_t, addr_t> datapump;

        // push synthetic incoming coap request data into netbuf via writer
        writer.write(buffer_16bit_delta, sizeof(buffer_16bit_delta));

        // Now take that synthetic netbuf data and simulates a transport input
        // on datapump
        datapump.transport_in(writer.netbuf(), 0);

        // set up message subject+observer
        request_context_t test_ctx;
        DecoderSubjectBase<experimental::ContextDispatcherHandler<request_context_t>> test(test_ctx);

        REQUIRE(!test_ctx.have_header());

        // now service the datapump with the 'test' DecoderSubject pushing out
        // to the message observer
        process_messageobserver(datapump, test);

        // Assert that ContextDispatcherHandler indeed parsed the incoming message
        REQUIRE(test_ctx.have_header());
        REQUIRE(test_ctx.header().message_id() == 0x0123);
    }
}
