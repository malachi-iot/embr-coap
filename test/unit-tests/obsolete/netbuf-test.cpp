#if OBSOLETE
#include <catch.hpp>

#include <coap/decoder/netbuf.h>
#include <exp/misc.h>
#include "exp/misc.hpp"
#include <exp/netbuf.h>
#include <embr/datapump.hpp>
#if __cplusplus >= 201103L
#include "platform/generic/malloc_netbuf.h"
#endif
#include "test-data.h"
#include "test-observer.h"

#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>

using namespace moducom::io::experimental;
using namespace embr::coap;
using namespace embr::experimental;

namespace std {

std::ostream& operator << ( std::ostream& os, Decoder::State const& value ) {
    os << get_description(value);
    return os;
}

}




template <class TNetBuf>
static void suite(NetBufDecoder<TNetBuf>& decoder)
{
    decoder.header();
    decoder.token();

    // NOTE: Flawed, because option_iterator sometimes wants
    // a reference for its parameter into NetBufDecoder.  Though in this
    // unit test, that is so far not the case
    option_iterator<NetBufDecoder<TNetBuf> > it(decoder);

    while(it.valid()) ++it;

    INFO(decoder.state());
    REQUIRE((decoder.state() == Decoder::Payload || decoder.state() == Decoder::Done));
}



TEST_CASE("netbuf+coap tests", "[netbuf-coap]")
{
    typedef estd::const_buffer chunk_t;

    SECTION("writer")
    {
        // FIX: This is a bad test, writer shouldn't be prepopulated!
        NetBufWriter<NetBufReadOnlyMemory> writer(buffer_16bit_delta);

        REQUIRE(writer.size() == sizeof(buffer_16bit_delta));
    }
    SECTION("reader")
    {
        NetBufReader<NetBufReadOnlyMemory> reader(buffer_16bit_delta);

        REQUIRE(reader.netbuf().length_unprocessed() == sizeof(buffer_16bit_delta));
    }
    SECTION("netbuf decoder")
    {
        NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_16bit_delta);

        Header header = decoder.header();

        REQUIRE(header.message_id() == 0x0123);
        REQUIRE(header.token_length() == 0);

        embr::coap::layer3::Token token = decoder.token();

        REQUIRE(token.size() == 0);

        // from RFC7252
        // "(Note that every message carries a token, even if it is of zero length.)"
        REQUIRE(decoder.state() == Decoder::OptionsStart);
    }
    SECTION("'simplest' (data) incoming decoder")
    {
        NetBufDecoder<NetBufReadOnlyMemory>  reader(buffer_simplest_request);

        reader.header();
        reader.token();
        reader.begin_option_experimental();

        REQUIRE(reader.netbuf().length_unprocessed() == sizeof(buffer_simplest_request));
    }
    SECTION("0-length value option")
    {
        NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_oversimplified_observe);

        Header header = decoder.header();

        decoder.token();

        decoder.begin_option_experimental();

        REQUIRE(decoder.state() == Decoder::Options);

        // FIX: clumsy call - have to make this *after* option header, but but also
        // after checking decoder.state()
        decoder.process_option_header_experimental();

        REQUIRE(decoder.option_number() == Option::Observe);

        chunk_t value = decoder.option();

        REQUIRE(value.size() == 0);
    }
#if __cplusplus >= 201103L
    SECTION("netbuf copy")
    {
        NetBufReadOnlyMemory buf1(buffer_simplest_request);
        NetBufDynamicExperimental buf2;

        coap_netbuf_copy(buf1, buf2);

        REQUIRE(buf2.length_processed() == 4);
        REQUIRE(memcmp(buf1.processed(), buf2.processed(), 4) == 0);
    }
    SECTION("netbuf copy ('skip' feature)")
    {
        NetBufReadOnlyMemory buf1(buffer_simplest_request);
        NetBufDynamicExperimental buf2;

        coap_netbuf_copy(buf1, buf2, 1);

        REQUIRE(buf2.length_processed() == 3);
        REQUIRE(memcmp(buf1.processed() + 1, buf2.processed(), 3) == 0);
    }
#endif
    SECTION("coap NetBufDecoder suite")
    {
        SECTION("No option, no payload")
        {
            NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_simplest_request);

            suite(decoder);
        }
        SECTION("With option (small), no payload")
        {
            NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_oversimplified_observe);

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
            NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_payload_only);

            suite(decoder);

            REQUIRE(decoder.state() == Decoder::Payload);
        }
        SECTION("With option, with payload")
        {
            NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_16bit_delta);

            suite(decoder);

            REQUIRE(decoder.state() == Decoder::Payload);
        }
    }
    SECTION("token decoding")
    {
        NetBufDecoder<NetBufReadOnlyMemory> decoder(buffer_with_token);

        Header header = decoder.header();

        REQUIRE(header.token_length() == 2);

        SECTION("layer2")
        {
            embr::coap::layer2::Token token;

            decoder.token(&token);

            REQUIRE(token.size() == 2);
            REQUIRE(token.data()[0] == 0x77);
            REQUIRE(token.data()[1] == 0x78);
        }
        SECTION("layer3")
        {
            embr::coap::layer3::Token token = decoder.token();

            REQUIRE(token.size() == 2);
            REQUIRE(token.data()[0] == 0x77);
            REQUIRE(token.data()[1] == 0x78);
        }
    }
    SECTION("embr based netbuf")
    {
        typedef embr::mem::experimental::NetBufDynamic<> netbuf_t;
        netbuf_t nb;

        // TODO: Consider instead a 'shrink' method like PBUF has
        nb.expand(sizeof(buffer_16bit_delta), true);
        REQUIRE(nb.data() != NULLPTR);
        memcpy(nb.data(), buffer_16bit_delta, sizeof(buffer_16bit_delta));

        // FIX: netbuf not reflecting proper size, need a kind of shrink operation
        // *or* decoder needs to be smart enough to handle that
        // *or* we have some kind of netbuf wrapper/reader
        NetBufDecoder<netbuf_t&> decoder(nb);

        // FIX: We actually pass an invalid size (128) in since nb.expand
        // is subject to a minimum allocation size.  Still, this works
        // since suite doesn't fully process payload
        suite(decoder);
    }
}
#endif