#include <catch.hpp>

#include <coap/decoder/netbuf.h>
#include <exp/netbuf.h>
#include <exp/datapump.hpp>
#include "platform/generic/malloc_netbuf.h"
#include "test-data.h"

using namespace moducom::io::experimental;
using namespace moducom::coap;

namespace std {

std::ostream& operator << ( std::ostream& os, Decoder::State const& value ) {
    os << get_description(value);
    return os;
}

}

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


static void suite(NetBufDecoder<NetBufMemory>& decoder)
{
    decoder.header();
    decoder.token();

    option_iterator<NetBufMemory, NetBufDecoder<NetBufMemory> > it(decoder, true);

    while(it.valid()) ++it;

    INFO(decoder.state());
    REQUIRE(decoder.state() == Decoder::OptionsDone);

    decoder.process_iterate();

    INFO(decoder.state());
    REQUIRE((decoder.state() == Decoder::Payload || decoder.state() == Decoder::Done));
}

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

        moducom::coap::layer3::Token token = decoder.token();

        REQUIRE(token.length() == 0);

        // from RFC7252
        // "(Note that every message carries a token, even if it is of zero length.)"
        REQUIRE(decoder.state() == Decoder::TokenDone);
    }
    SECTION("'simplest' (data) incoming decoder")
    {
        NetBufDecoder<NetBufMemory>  reader(buffer_simplest_request);

        reader.header();
        reader.token();
        reader.begin_option_experimental();

        REQUIRE(reader.netbuf().length_unprocessed() == sizeof(buffer_simplest_request));
    }
    SECTION("0-length value option")
    {
        typedef moducom::pipeline::MemoryChunk::readonly_t chunk_t;
        NetBufDecoder<NetBufMemory> decoder(buffer_oversimplified_observe);

        Header header = decoder.header();

        decoder.token();

        decoder.begin_option_experimental();

        REQUIRE(decoder.state() == Decoder::Options);

        // FIX: clumsy call - have to make this *after* option header, but but also
        // after checking decoder.state()
        decoder.process_option_header_experimental();

        REQUIRE(decoder.option_number() == Option::Observe);

        chunk_t value = decoder.process_option_value_experimental();

        REQUIRE(value.length() == 0);
    }
    SECTION("netbuf copy")
    {
        NetBufMemory buf1(buffer_simplest_request);
        NetBufDynamicExperimental buf2;

        netbuf_copy(buf1, buf2);

        REQUIRE(buf2.length_processed() == 4);
        REQUIRE(memcmp(buf1.processed(), buf2.processed(), 4) == 0);
    }
    SECTION("netbuf copy ('skip' feature)")
    {
        NetBufMemory buf1(buffer_simplest_request);
        NetBufDynamicExperimental buf2;

        netbuf_copy(buf1, buf2, 1);

        REQUIRE(buf2.length_processed() == 3);
        REQUIRE(memcmp(buf1.processed() + 1, buf2.processed(), 3) == 0);
    }
    SECTION("coap NetBufDecoder suite")
    {
        SECTION("No option, no payload")
        {
            NetBufDecoder<NetBufMemory> decoder(buffer_simplest_request);

            suite(decoder);
        }
        SECTION("With option (small), no payload")
        {
            NetBufDecoder<NetBufMemory> decoder(buffer_oversimplified_observe);

            suite(decoder);
        }
        /*
        SECTION("With option, no payload")
        {
            NetBufDecoder<NetBufMemory> decoder(buffer_oversimplified_observe);

            suite(decoder);
        } */
        SECTION("No option, with payload")
        {
            NetBufDecoder<NetBufMemory> decoder(buffer_payload_only);

            suite(decoder);

            REQUIRE(decoder.state() == Decoder::Payload);
        }
        SECTION("With option, with payload")
        {
            NetBufDecoder<NetBufMemory> decoder(buffer_16bit_delta);

            suite(decoder);

            REQUIRE(decoder.state() == Decoder::Payload);
        }
    }
}
