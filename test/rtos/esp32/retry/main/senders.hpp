#pragma once

#include <coap/decoder/streambuf.hpp>

// DEBT
using namespace embr;

// DEBT: Relative of embr::coap::experimental::get_header - 
// but this 'until' feels closer to what we really want
template <class Streambuf>
bool until(coap::StreambufDecoder<Streambuf>& decoder, coap::Decoder::State state)
{
    coap::iterated::decode_result r;

    do
    {
        r = decoder.process_iterate_streambuf();
    }
    while(decoder.state() != state && r.eof == false);

    return r.eof == false;
}


template <class IStreambuf, class OStreambuf>
bool build_ack(
    embr::coap::StreambufDecoder<IStreambuf>& decoder,
    embr::coap::StreambufEncoder<OStreambuf>& encoder
    )
{
    until(decoder, coap::Decoder::HeaderDone);
    coap::Header header = decoder.header_decoder();
    until(decoder, coap::Decoder::TokenDone);
    const uint8_t* token = decoder.token_decoder().data();

    header.type(coap::Header::Types::Acknowledgement);
    header.code(coap::Header::Code::Valid);

    encoder.header(header);
    encoder.token(token, header.token_length());
    encoder.finalize();

    return true;
}


template <class IStreambuf, class OStreambuf>
bool build_con(
    embr::coap::StreambufDecoder<IStreambuf>& decoder,
    embr::coap::StreambufEncoder<OStreambuf>& encoder
    )
{
    until(decoder, coap::Decoder::HeaderDone);
    coap::Header header = decoder.header_decoder();
    until(decoder, coap::Decoder::TokenDone);
    const uint8_t* token = decoder.token_decoder().data();

    if(header.type() == coap::Header::Reset)  return false;

    header.type(coap::Header::Types::Confirmable);
    header.code(coap::Header::Code::Valid);
    // DEBT: Pretty lousy way to generate a different message ID
    header.message_id(header.message_id() + 1);

    encoder.header(header);
    encoder.token(token, header.token_length());
    encoder.finalize();

    return true;
}