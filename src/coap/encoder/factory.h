/**
 * @file
 * Depends on external party defining "make_encoder"
 */
#pragma once

#include <coap/context.h>
#include <coap/header.h>

namespace moducom { namespace coap {

template <class TStreambuf, class TStreambufEncoderImpl>
class StreambufEncoder;

template <bool inline_token, class TStreambuf, class TStreambufEncoderImpl>
inline void build_reply(
    const TokenAndHeaderContext<inline_token>& context, 
    StreambufEncoder<TStreambuf, TStreambufEncoderImpl>& encoder, uint8_t code)
{
    Header header = context.header();
    auto token = context.token();

    header.code(code);
    header.type(Header::TypeEnum::Acknowledgement);

    encoder.header(header);
    encoder.token(token);
}

// Expects TContext to be/conform to:
// moducom::coap::IncomingContext
// TODO: better suited to cpp/hpp - be nice to non-inline it
// NOTE: leans heavily on RVO, at least as much as 'make_encoder' itself does
template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(const TContext& context, uint8_t code)
{
    typename TContext::encoder_type encoder = make_encoder(context);

    build_reply(context, encoder, code);

    return encoder;
}

}}