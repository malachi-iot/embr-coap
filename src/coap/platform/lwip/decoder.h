#pragma once

#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/transport.h>
#include "../../decoder/streambuf.h"

namespace embr { namespace coap {

// Consider moving this into coap-lwip area if we can nail down bump vs not bump
// reference count on pbuf
StreambufDecoder<embr::lwip::upgrading::ipbuf_streambuf> make_decoder(embr::lwip::Pbuf* netbuf)
{
    // TODO: No copy-constructor version yet, may want to make one
    //StreambufDecoder<streambuf_type> decoder(in);

    // NOTE: This auto-bumps pbuf ref count, which we want since
    // StreambufDecoder on destruction will also destroy its netbuf,
    // which in turn does a pbuf_free.  In other words, Streambuf housekeeping
    // bumps then de-bumps the reference, we come back to where we started,
    // which is it's someone else's responsibilty to fully free the pbuf
    return StreambufDecoder<embr::lwip::upgrading::ipbuf_streambuf>(netbuf->pbuf());
}

}}