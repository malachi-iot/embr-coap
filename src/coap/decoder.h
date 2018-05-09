#pragma once

#include "../coap-decoder.h"

namespace moducom { namespace coap {

// standalone Decoder works well enough, so this is largely just a netbuf-capable
// wrapper around it
template <class TNetBuf>
class NetBufDecoder : public Decoder
{
    typedef TNetBuf netbuf_t;
    typedef Decoder base_t;

protected:
    netbuf_t netbuf;
    // FIX: intermediate chunk until context has a value instead of a ref for its chunk
    moducom::pipeline::MemoryChunk::readonly_t chunk;
    Context context;

public:
    NetBufDecoder(const netbuf_t& netbuf) :
        netbuf(netbuf),
        chunk(netbuf.processed(), netbuf.length_processed()),
        context(chunk, netbuf.end())
    {}

    bool process_iterate()
    {
        // TODO: Will know how to advance through netbuf
        return base_t::process_iterate(context);
    }
};

}}
