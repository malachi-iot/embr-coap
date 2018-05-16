#include <catch.hpp>

#include <coap/decoder/netbuf.h>
#include <exp/netbuf.h>
#include "test-data.h"

using namespace moducom::io::experimental;
using namespace moducom::coap;


// simplistic memorychunk-mapped NetBuf.  Eventually put this into mcmem itself
class NetBufMemory :
        public NetBufMemoryTemplate<moducom::pipeline::MemoryChunk >
{
    typedef moducom::pipeline::MemoryChunk chunk_t;
    typedef NetBufMemoryTemplate<chunk_t> base_t;

public:
    template <size_t N>
    NetBufMemory(uint8_t (&a) [N]) :
        base_t(chunk_t(a, N))
    {

    }
};


TEST_CASE("netbuf+coap tests", "[netbuf-coap]")
{
    SECTION("writer")
    {
        // FIX: This is a bad test, writer shouldn't be prepopulated!
        NetBufWriter<NetBufMemory> writer(buffer_16bit_delta);

        REQUIRE(writer.size() == sizeof(buffer_16bit_delta));
    }
    SECTION("reader")
    {
        NetBufReader<NetBufMemory> reader(buffer_16bit_delta);

        REQUIRE(reader.netbuf().length_unprocessed() == sizeof(buffer_16bit_delta));
    }
    SECTION("netbuf decoder")
    {
        NetBufDecoder<NetBufMemory> decoder(buffer_16bit_delta);
    }
}
