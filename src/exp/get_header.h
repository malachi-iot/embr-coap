#include "../coap/internal/factory.h"

#include "../coap/decoder/result.h"
#include "../coap/internal/root.h"
#include "../coap/header.h"

namespace embr { namespace coap { namespace experimental {

// DEBT: Helper function - somewhat problematic.  It free-floating
// out here with ADL making it consume all buffers is a little concerning.
// However, DecoderFactory specializations will only take on certain TBuffers
template <class TBuffer>
inline Header get_header(TBuffer buffer)
{
    auto decoder = DecoderFactory<TBuffer>::create(buffer);

    iterated::decode_result r;

    do
    {
        r = decoder.process_iterate_streambuf();
    }
    while(!(r.eof || decoder.state() == internal::Root::HeaderDone));

    return decoder.header_decoder();
}

}}}