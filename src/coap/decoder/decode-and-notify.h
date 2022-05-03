#pragma once

#include <estd/variant.h>

#include "context.h"

namespace embr { namespace coap {

class Decoder;

template <class TStreambuf>
class StreambufDecoder;

// Used to call this 'internal' but ADL causes calls to flow here
// unexpectedly
namespace iterated {

// TSubject = one of the new embr::*::subject classes
// NOTE: Not perfect location, a global function, but not terrible either.  Perhaps
// attaching it to Decoder, since TSubject is entirely ephemeral anyway?
// Also be advised this actually does a process iterate as well as a notify
template <class TSubject, class TContext>
decode_result decode_and_notify(
    Decoder& decoder,
    TSubject& subject,
    internal::DecoderContext& context,
    TContext& app_context);

template <class TSubject, class TStreambuf, class TContext>
decode_result decode_and_notify(
    StreambufDecoder<TStreambuf>& decoder,
    TSubject& subject,
    TContext& app_context);


///
/// \tparam TSubject
/// \param decoder
/// \param subject
/// \param context
/// \return decode_result reflecting eof, waitstate or bad decode
template <class TSubject>
inline decode_result decode_and_notify(Decoder& decoder, TSubject& subject, internal::DecoderContext& context)
{
    estd::monostate empty_app_context;

    return iterated::decode_and_notify(decoder, subject, context, empty_app_context);
}

template <class TSubject, class TStreambuf>
inline decode_result decode_and_notify(StreambufDecoder<TStreambuf>& decoder, TSubject& subject)
{
    estd::monostate empty_app_context;

    return iterated::decode_and_notify(decoder, subject, empty_app_context);
}


}

/// Auto iterating decoder/notifier
/// Runs until a stopping point is encountered
/// \tparam TSubject
/// \tparam TStreambuf
/// \tparam TContext
/// \param decoder
/// \param subject
/// \param app_context
/// \return @see iterated::decode_result
template <class TSubject, class TStreambuf, class TContext>
inline iterated::decode_result decode_and_notify(
    StreambufDecoder<TStreambuf>& decoder,
    TSubject& subject,
    TContext& app_context)
{
    iterated::decode_result r;

    do
    {
        r = iterated::decode_and_notify(decoder, subject, app_context);
    }
    while((r.failure | r.waitstate | r.done) == 0);

    return r;
}


/// Auto iterating decoder/notifier
/// Runs until a stopping point is encountered
/// \tparam TSubject
/// \tparam TStreambuf
/// \tparam TContext
/// \param decoder
/// \param subject
/// \param app_context
/// \return iterated::decode_result indicates reason for stopping
template <class TSubject, class TContext>
inline iterated::decode_result decode_and_notify(
    Decoder& decoder,
    TSubject& subject,
    internal::DecoderContext& context,
    TContext& app_context)
{
    iterated::decode_result r;

    do
    {
        r = iterated::decode_and_notify(decoder, subject, context, app_context);
    }
    while((r.failure | r.waitstate | r.done) == 0);

    return r;
}


/// Auto iterating decoder/notifier.  Runs until a stopping point is encountered
/// Uses an empty monostate for app_context
/// \return iterated::decode_result indicates reason for stopping
template <class TSubject, class TStreambuf>
inline iterated::decode_result decode_and_notify(StreambufDecoder<TStreambuf>& decoder, TSubject& subject)
{
    estd::monostate empty_app_context;

    return coap::decode_and_notify(decoder, subject, empty_app_context);
}

/// Auto iterating decoder/notifier.  Runs until a stopping point is encountered
/// Uses an empty monostate for app_context
/// \return iterated::decode_result indicates reason for stopping
template <class TSubject>
inline iterated::decode_result decode_and_notify(Decoder& decoder, TSubject& subject, internal::DecoderContext& context)
{
    estd::monostate empty_app_context;

    return coap::decode_and_notify(decoder, subject, context, empty_app_context);
}


// NOTE: Not functional yet
#ifdef FEATURE_CPP_MOVESEMANTIC
/// Auto iterating decoder/notifier.  Runs until a stopping point is encountered
/// Uses an empty monostate for app_context
/// \return iterated::decode_result indicates reason for stopping
template <class TSubject>
iterated::decode_result decode_and_notify(Decoder& decoder, TSubject&& subject, internal::DecoderContext& context)
{
    estd::monostate empty_app_context;

    return coap::decode_and_notify(decoder, std::move(subject), context, empty_app_context);
}
#endif

}}