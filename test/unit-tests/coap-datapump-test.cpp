#include <catch.hpp>

#include <platform/generic/datapump-messageobserver.hpp>
#include "test-data.h"

using namespace moducom::coap;

// +++ just to test compilation, eliminate once decent unit tests for
// DecoderSubjectBase is in place
static IncomingContext test_ctx;
static DecoderSubjectBase<experimental::ContextDispatcherHandler> test(test_ctx);
// ---

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
        typedef moducom::io::experimental::NetBufDynamicMemory<> netbuf_t;
        moducom::io::experimental::NetBufWriter<netbuf_t> writer;
        DataPump<netbuf_t, addr_t> datapump;
    }
}
