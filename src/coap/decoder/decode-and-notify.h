#pragma once

#include <coap/decoder.h>
#include <coap/decoder/streambuf.h>

namespace embr { namespace coap {

namespace internal {

// TSubject = one of the new embr::*::subject classes
// NOTE: Not perfect location, a global function, but not terrible either.  Perhaps
// attaching it to Decoder, since TSubject is entirely ephemeral anyway?
// Also be advised this actually does a process iterate as well as a notify
template <class TSubject, class TContext>
bool decode_and_notify(
    Decoder& decoder,
    TSubject& subject,
    Decoder::Context& context,
    TContext& app_context);

template <class TSubject>
bool decode_and_notify(Decoder& decoder, TSubject& subject, Decoder::Context& context)
{
    int fake_app_context;

    return decode_and_notify(decoder, subject, context, fake_app_context);
}

template <class TSubject, class TStreambuf>
bool decode_and_notify(StreambufDecoder<TStreambuf>& decoder, TSubject& subject)
{
    int fake_app_context;

    return decode_and_notify(decoder, subject, fake_app_context);
}


}

}}