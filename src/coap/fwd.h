#pragma once

#include "internal/header/code.h"
#include "internal/header/enum.h"

#include "encoder/concepts.h"

namespace embr { namespace coap {

namespace impl {

// DEBT: Pretty sure we don't need an explicit coap finalization like this, but rather
// a transport-level one.  So, making this default to not implemented and very much moving
// towards phasing it out completely
template<class TStreambuf>
struct StreambufEncoderImpl;

}

class Decoder;

template <class TStreambuf, class TStreambufEncoderImpl = impl::StreambufEncoderImpl<TStreambuf> >
class StreambufEncoder;

template <class TStreambuf>
class StreambufDecoder;

template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(TContext& context,
    internal::header::Code code, header::Types type = header::Types::Acknowledgement);

}}