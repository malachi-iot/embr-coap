#include <catch.hpp>

#include <coap/decoder/netbuf.h>
#include <exp/netbuf.h>
#include "test-data.h"

using namespace moducom::io::experimental;
using namespace moducom::coap;


// simplistic memorychunk-mapped NetBuf.  Eventually put this into mcmem itself
// FIX: this one is hardwired to read operations - not a crime, but needs more
// architectual planning to actually be used elsewhere
class NetBufMemory :
        public NetBufMemoryTemplate<moducom::pipeline::MemoryChunk::readonly_t >
{
    typedef moducom::pipeline::MemoryChunk::readonly_t chunk_t;
    typedef NetBufMemoryTemplate<chunk_t> base_t;

public:
    template <size_t N>
    NetBufMemory(const uint8_t (&a) [N]) :
        base_t(chunk_t(a, N))
    {

    }

    // FIX: hard wiring this to a read-oriented NetBuf
    // there's some mild indication this isn't ideal since conjunctive decoder
    // still needs to maintain a pos to read through this
    size_t length_processed() const
    {
        return base_t::_chunk.length();
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

        Header header = decoder.header();

        REQUIRE(header.message_id() == 0x0123);

        // FIX: this has a problem because actually TokenDone isn't evaluated if
        // no token is present.  We're going to change that, because of this wording
        // from RFC7252
        // "(Note that every message carries a token, even if it is of zero length.)"
        moducom::coap::layer3::Token token = decoder.process_token_experimental();

        REQUIRE(token.length() == 0);
    }
    SECTION("0-length value option")
    {
        typedef moducom::pipeline::MemoryChunk::readonly_t chunk_t;
        NetBufDecoder<NetBufMemory> decoder(buffer_oversimplified_observe);

        Header header = decoder.header();

        decoder.process_token_experimental();

        decoder.begin_option_experimental();

        REQUIRE(decoder.state() == Decoder::Options);

        // FIX: clumsy call - have to make this *after* option header, but but also
        // after checking decoder.state()
        decoder.process_option_header_experimental();

        chunk_t value = decoder.process_option_value_experimental();
        //decoder.option_decoder().state()
    }
}
