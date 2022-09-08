// See README.md
#pragma once

// Different than parent factory.h, this one focuses on specializations
// to create encoders and decoders depending on the buffers at play

namespace embr { namespace coap { namespace experimental {

template <class TBuffer>
struct DecoderFactory;

template <class TBuffer>
struct EncoderFactory;

}}}