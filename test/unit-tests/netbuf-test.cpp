#include <catch.hpp>

#include <coap/decoder/netbuf.h>
#include <exp/netbuf.h>
#include "test-data.h"

using namespace moducom::io::experimental;

/*
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
}; */


TEST_CASE("netbuf+coap tests", "[netbuf-coap]")
{
    SECTION("decoder")
    {
        //NetBufWriter<NetBufMemory> writer(buffer_16bit_delta);
    }
}
