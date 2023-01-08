/**
 * @file
 * Depends on external party defining "make_encoder"
 */
#pragma once

#include <coap/fwd.h>

#include <coap/context.h>
#include <coap/header.h>

#include "../internal/factory.h"

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

inline void build_reply_extra(tags::header_context, Header::Code code, bool)
{

}

// trailing int guarantees higher precedence, thus disambiguating call
inline void build_reply_extra(ExtraContext& context, Header::Code code, int)
{
    context.response_code = code;
}

inline Header build_reply_header(const HeaderContext& context, Header::Code code, Header::Types type)
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
        estd::is_base_of<tags::header_context, TContext>::value &&
        !estd::is_base_of<internal::ExtraContext, TContext>::value
    >::type>
inline void build_reply(
    const TContext& context,
    StreambufEncoder<TStreambuf, TStreambufEncoderImpl>& encoder,
    Header::Code code, header::Types type = Header::Acknowledgement)
{
    Header header = internal::build_reply_header(context, code, type);
    internal::build_reply_encoder(encoder, header, context.token());
}


template <class TContext, class TStreambuf, class TStreambufEncoderImpl, typename enable =
    typename estd::enable_if<
        estd::is_base_of<tags::token_context, TContext>::value &&
        estd::is_base_of<tags::header_context, TContext>::value &&
        estd::is_base_of<internal::ExtraContext, TContext>::value
    >::type>
inline void build_reply(
    TContext& context,
    StreambufEncoder<TStreambuf, TStreambufEncoderImpl>& encoder,
    Header::Code code, header::Types type = Header::Acknowledgement)
{
    Header header = internal::build_reply_header(context, code, type);
    internal::build_reply_extra(context, code, 1);
    internal::build_reply_encoder(encoder, header, context.token());
}

// DEBT: make_encoder now on the way out.  Keeping around in case we want to specialize
// directly on that method, but main customization should now occur in TContext::encoder_factory
template <class TContext>
inline typename TContext::encoder_type make_encoder(const TContext&)
{
    return TContext::encoder_factory::create();
}

// Expects TContext to be/conform to:
// embr::coap::IncomingContext
// TODO: better suited to cpp/hpp - be nice to non-inline it
// NOTE: leans heavily on RVO, at least as much as 'make_encoder' itself does
// DEBT: Put in an is_base_of to enforce IncomingContext, and even that is not enough because
// 'encoder_type' expectation is not fully solidified just yet
template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(TContext& context,
    internal::header::Code code, header::Types type)
{
    typename TContext::encoder_type encoder = make_encoder(context);

    build_reply(context, encoder, code, type);

    return encoder;
}

template <class TContext>
typename estd::enable_if<
    estd::is_base_of<tags::incoming_context, TContext>::value &&
    estd::is_base_of<internal::ExtraContext, TContext>::value, void>
    ::type
auto_reply(TContext& context, typename TContext::encoder_type& encoder)
{
    if(context.flags.response_sent) return;

    Header::Code code = context.response_code.has_value() ?
        context.response_code.value() :
        Header::Code::NotFound;

    build_reply(context, encoder, code);
    context.reply(encoder);
}



}}
