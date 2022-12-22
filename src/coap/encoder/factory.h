/**
 * @file
 * Depends on external party defining "make_encoder"
 */
#pragma once

#include <coap/fwd.h>

#include <coap/context.h>
#include <coap/header.h>

namespace embr { namespace coap {

namespace internal {

/*
 * always consumes call
template <class TContext>
inline void build_reply_extra(TContext, Header::Code::Codes code)
{

}

 * below build_reply_extras do not end up defining this call, even if adding template <>
template <class TContext, typename TPrecedent>
inline void build_reply_extra(TContext, Header::Code::Codes code, TPrecedent = 1);
 */

inline void build_reply_extra(tags::header_context, Header::Code::Codes code, bool)
{

}

// trailing int guarantees higher precedence, thus disambiguating call
inline void build_reply_extra(ExtraContext& context, Header::Code::Codes code, int)
{
    context.response_code = code;
}

inline Header build_reply_header(HeaderContext& context, Header::Code::Codes code, header::Types type)
{
    Header header = context.header();

    header.code(code);
    header.type(type);

    return header;
}


template <class TStreambuf, class TStreambufEncoderImpl>
inline void build_reply_encoder(
    StreambufEncoder<TStreambuf, TStreambufEncoderImpl>& encoder,
    Header header, layer3::Token token)
{
    encoder.header(header);
    encoder.token(token);
}

}


/**
 * build a minimal reply populating header and token - no options or payload
 * @param context context with which to lift header and token
 * @param encoder StreambufEncoder to create output with
 * @param code CoAP header code
 */
template <class TContext, class TStreambuf, class TStreambufEncoderImpl, typename enable =
    typename estd::enable_if<
        estd::is_base_of<tags::token_context, TContext>::value &&
        estd::is_base_of<tags::header_context, TContext>::value
    >::type>
inline void build_reply(
    TContext& context,
    StreambufEncoder<TStreambuf, TStreambufEncoderImpl>& encoder,
    Header::Code::Codes code, header::Types type = Header::Acknowledgement)
{
    Header header = internal::build_reply_header(context, code, type);
    internal::build_reply_extra(context, code, 1);
    internal::build_reply_encoder(encoder, header, context.token());
}

// DEBT: Just as below, a true EncoderFactory would be better
template <class TContext>
typename TContext::encoder_type make_encoder(TContext&);

// Expects TContext to be/conform to:
// embr::coap::IncomingContext
// TODO: better suited to cpp/hpp - be nice to non-inline it
// NOTE: leans heavily on RVO, at least as much as 'make_encoder' itself does
// DEBT: Put in an is_base_of to enforce IncomingContext, and even that is not enough because
// 'encoder_type' expectation is not fully solidified just yet
template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(TContext& context,
    Header::Code::Codes code, header::Types type)
{
    // DEBT: make_encoder is a pretty flimsy factory.  We're likely better off with the still experimental
    // fully specialized struct EncoderFactory in retry area
    typename TContext::encoder_type encoder = make_encoder(context);

    build_reply(context, encoder, code, type);

    return encoder;
}

}}
