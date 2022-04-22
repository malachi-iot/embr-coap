#pragma once

#include "context.h"

namespace moducom { namespace coap {


// in C# we'd do an IEncoderFactory, in C++ for tiny code footprint
// we'll instead do a special override of make_encoder.  Important since
// the particulars of how an encoder+streambuf is initialized changes
// for the underlying streambuf+application in question

// TODO: Best if we can decouple from LwipContext somehow.  Perhaps
// specialize on a TContext::encoder_type?
inline LwipContext::encoder_type make_encoder(const LwipContext&)
{
    // stock-standard size is 256, which is generally too large
    // for many CoAP scenarios but still small enough to throw around
    // in a memory constrained system.  One can (and should) specialize
    // their context for more specificity
    return LwipContext::encoder_type(256);
}

namespace impl {

template <>
struct StreambufEncoderImpl<::embr::lwip::upgrading::opbuf_streambuf>
{
    typedef typename estd::remove_const<::embr::lwip::upgrading::opbuf_streambuf>::type streambuf_type;
    typedef typename streambuf_type::size_type size_type;

    static void finalize(streambuf_type* streambuf)
    {
        streambuf->shrink();
    }
};

}

}}