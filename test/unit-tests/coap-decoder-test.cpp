#include <catch.hpp>

#include "coap/decoder.h"
#include "coap/decoder/streambuf.hpp"
#include "test-data.h"
//#include "../mc/pipeline.h"

using namespace embr::coap;
//using namespace moducom::pipeline;

// +++ EXPERIMENTAL
template <class TStreambuf>
inline estd::span<const typename TStreambuf::char_type> sgetn(TStreambuf& s, estd::streamsize len)
{
    len = estd::min(s.in_avail(), len);

    // TODO: consider using estd::optional if len <= 0
    estd::span<const typename TStreambuf::char_type> span(s.gptr(), len);

    s.pubseekoff(len, estd::ios_base::cur);

    return span;
}
// ---


typedef estd::span<const uint8_t> buffer_type;

template <unsigned N>
static void completion_state(Decoder& decoder, const uint8_t (&data) [N])
{
    buffer_type b(data);
    Decoder::Context context(b, true);

    while(!decoder.process_iterate(context).done);
}

TEST_CASE("CoAP decoder tests", "[coap-decoder]")
{
    typedef estd::span<const uint8_t> ro_chunk_t;

    ro_chunk_t buffer_in(buffer_16bit_delta);


    //layer3::SimpleBufferedPipeline net_in(buffer_in);

    SECTION("Basic test 1")
    {
        OptionDecoder decoder;
        OptionDecoder::Holder option;

        // pretty much ready to TRY testing, just need to load in appropriate data into buffer_in
        //decoder.process_iterate(net_in, &option);
    }
    SECTION("Payload only test")
    {
        Decoder decoder;
        ro_chunk_t chunk(buffer_payload_only);
        Decoder::Context context(chunk, true);

        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Header);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::HeaderDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::TokenDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::OptionsStart);
        REQUIRE(decoder.process_iterate(context).eof == false);
        // FIX: Really shouldn't have an Options stage with an overall
        // decoder if no options present.  This may be a case where a little
        // code overlap is desired - just because OptionsDecoder *can* detect
        // payload marker doesn't need we *should* utilize it, though reuse
        // dictates we must consider it
        REQUIRE(decoder.state() == Decoder::OptionsDone);
        // FIX: in any case, we shouldn't be done with the buffer at this point,
        // so failing unit test here is a bug
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Payload);
        REQUIRE(decoder.process_iterate(context).eof == true);
        REQUIRE(decoder.state() == Decoder::PayloadDone);
        REQUIRE(decoder.completion_state().payloadPresent);
        REQUIRE(decoder.process_iterate(context).eof == true);
        REQUIRE(decoder.state() == Decoder::Done);
    }
    SECTION("16 bit delta test")
    {
        Decoder decoder;
        ro_chunk_t chunk(buffer_16bit_delta);
        Decoder::Context context(chunk, true);

        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Header);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::HeaderDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::TokenDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::OptionsStart);
        // kicks off option decoding itself, first stops after
        // length is processed
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Options);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionLengthDone);
#endif
        REQUIRE(decoder.option_length() == 1);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
        REQUIRE(decoder.process_iterate(context) == false);
        // Would really prefer OptionDeltaDone or OptionLengthAndDeltaDone were exposed here
#endif
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::ValueStart);
        REQUIRE(decoder.option_number() == 270);
        REQUIRE(decoder.option_decoder().option_delta() == 270);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Options);
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionValueDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Options);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionDeltaAndLengthDone);
#endif
        REQUIRE(decoder.option_length() == 2);
        REQUIRE(decoder.option_number() == 271);
        REQUIRE(decoder.option_decoder().option_delta() == 1);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
        REQUIRE(decoder.process_iterate(context) == false);
        REQUIRE(decoder.state() == Decoder::Options);
#endif
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::ValueStart);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::Options);
        REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionValueDone);
        REQUIRE(decoder.process_iterate(context).eof == false);
        REQUIRE(decoder.state() == Decoder::OptionsDone);
    }
    SECTION("Parity test with bronze-star project (incoming request header)")
    {
        Decoder decoder;
        // tkl of 4
        uint8_t header[] = { 0x44, 0x01, 0x6F, 0x34 };
        ro_chunk_t chunk(header);

        Decoder::Context context(chunk, true);

        REQUIRE(decoder.state() == Decoder::Uninitialized);
        decoder.process_iterate(context);
        REQUIRE(decoder.state() == Decoder::Header);
        decoder.process_iterate(context);
        REQUIRE(decoder.state() == Decoder::HeaderDone);
        decoder.process_iterate(context);
        SECTION("without token")
        {
            // DEBT: At this point I think we want to detect a waitstate or similar since
            // token data is not appearing and last chunk was set to true
        }
        SECTION("with token")
        {
            // DEBT: Since above debt is in place, we'll plunge forward acting like last_chunk
            // was false
            uint8_t token[] = { 0x1, 0x2, 0x3, 0x4 };

            Decoder::Context context_token(token, true);

            while(!decoder.process_iterate(context_token).done);

            // DEBT: Definitely incongruous looking for post-done uninitialized
            // state yet also looking for payload not present flag
            REQUIRE(decoder.state() == Decoder::Uninitialized);
            REQUIRE(decoder.completion_state().payloadPresent == false);
        }
    }
    SECTION("streambuf decoder")
    {
        typedef char char_type;
        typedef estd::internal::streambuf<
                estd::internal::impl::in_span_streambuf<char_type> > streambuf_type;

        estd::span<char_type> span((char_type*)buffer_16bit_delta, sizeof(buffer_16bit_delta));
        typedef estd::span<const char_type> const_buffer;

        // FIX: in theory we can std::forward buffer directly down into out_span_streambuf, but it isn't happy
#ifdef FEATURE_MCCOAP_EXPCONTEXT
        embr::coap::StreambufDecoder<streambuf_type> decoder(span.size(), span);
#else
        embr::coap::StreambufDecoder<streambuf_type> decoder(span);
#endif

#ifdef FEATURE_MCCOAP_EXPCONTEXT
        SECTION("legacy chunk-style")
        {
            // somewhat a copy/paste from "16 bit delta test"
            REQUIRE(!decoder.process_iterate());
            REQUIRE(decoder.state() == Decoder::Header);
            REQUIRE(!decoder.process_iterate());
            REQUIRE(decoder.state() == Decoder::HeaderDone);
            uint16_t message_id = decoder.header_decoder().message_id();
            REQUIRE(message_id == 0x0123);

            REQUIRE(!decoder.process_iterate());
            REQUIRE(decoder.state() == Decoder::TokenDone);
            REQUIRE(!decoder.process_iterate());
            REQUIRE(decoder.state() == Decoder::OptionsStart);

            REQUIRE(!decoder.process_iterate());
            REQUIRE(decoder.state() == Decoder::Options);
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionLengthDone);
            REQUIRE(decoder.option_length() == 1);
        }
#endif
        SECTION("streambuf native style")
        {
            // somewhat a copy/paste from "16 bit delta test"
            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Header);
            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::HeaderDone);
            uint16_t message_id = decoder.header_decoder().message_id();
            REQUIRE(message_id == 0x0123);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::TokenDone);
            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::OptionsStart);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Options);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionLengthDone);
#endif
            REQUIRE(decoder.option_length() == 1);

#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
            REQUIRE(!decoder.process_iterate_streambuf());
            // Would really prefer OptionDeltaDone or OptionLengthAndDeltaDone were exposed here
#endif
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::ValueStart);
            REQUIRE(decoder.option_number() == 270);
            REQUIRE(decoder.option_decoder().option_delta() == 270);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Options);
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionValueDone);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Options);
#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionDeltaAndLengthDone);
#endif
            REQUIRE(decoder.option_length() == 2);
            REQUIRE(decoder.option_number() == 271);
            REQUIRE(decoder.option_decoder().option_delta() == 1);

#if !FEATURE_MCCOAP_SUCCINCT_OPTIONDECODE
            REQUIRE(!decoder.process_iterate_streambuf());
            REQUIRE(decoder.state() == Decoder::Options);
#endif
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::ValueStart);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Options);
            REQUIRE(decoder.option_decoder().state() == OptionDecoder::OptionValueDone);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::OptionsDone);

            REQUIRE(!decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::Payload);

            auto& rdbuf = *decoder.rdbuf();

            const_buffer payload = sgetn(rdbuf, rdbuf.in_avail());

            const_buffer original_payload((char_type*)&buffer_16bit_delta[12], sizeof(buffer_16bit_delta) - 12);

            REQUIRE(payload.size() == 7);
            REQUIRE(payload[0] == buffer_16bit_delta[12]);

            //REQUIRE(payload == original_payload);
            //decoder.rdbuf()->gptr();

            REQUIRE(decoder.process_iterate_streambuf().eof);
            REQUIRE(decoder.state() == Decoder::PayloadDone);
        }
        SECTION("wait state")
        {
        }
    }
    SECTION("payload/no_payload completion state")
    {
        Decoder decoder;

        SECTION("buffer_16bit_delta")
        {
            completion_state(decoder, buffer_16bit_delta);

            REQUIRE(decoder.completion_state().payloadPresent == true);
        }
        SECTION("buffer_plausible")
        {
            completion_state(decoder, buffer_plausible);

            REQUIRE(decoder.completion_state().payloadPresent == true);
        }
        SECTION("buffer_with_token")
        {
            completion_state(decoder, buffer_with_token);

            REQUIRE(decoder.completion_state().payloadPresent == true);
        }
        SECTION("buffer_oversimplified_observe")
        {
            completion_state(decoder, buffer_oversimplified_observe);

            REQUIRE(decoder.completion_state().payloadPresent == false);
        }
        SECTION("buffer_simplest_request")
        {
            completion_state(decoder, buffer_simplest_request);

            REQUIRE(decoder.completion_state().payloadPresent == false);
        }
        SECTION("buffer_ack")
        {
            completion_state(decoder, buffer_ack);

            REQUIRE(decoder.completion_state().payloadPresent == false);
        }
    }
}
