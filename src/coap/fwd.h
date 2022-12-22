#pragma once

#include "internal/header/code.h"
#include "internal/header/enum.h"

namespace embr { namespace coap {

template <class TStreambuf, class TStreambufEncoderImpl>
class StreambufEncoder;

template <class TContext>
inline typename TContext::encoder_type make_encoder_reply(TContext& context,
    internal::header::Code::Codes code, header::Types type = header::Types::Acknowledgement);

}}