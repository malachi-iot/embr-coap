#pragma once

namespace moducom { namespace coap {

template <class TAddr>
struct address_traits
{
    // special compare which only compares pure addresses and excludes things like port number
    static bool equals_fromto(const TAddr& from_addr, const TAddr& to_addr)
    {
        return from_addr == to_addr;
    }
};

}}

namespace mb_iot {

// can and should be used also to do semi-partial coap netbuf copies, so that we can
// simulate a header rewrite.  As such, be mindful that default behavior is for this to NOT
// reset to 'first' but instead to copy as-is from current positions
// skip represents number of bytes from beginning of source to skip before
// initiating copy.  NOTE: this skip may obviate reset flag
template <class TNetBuf, class TNetBuf2>
void coap_netbuf_copy(TNetBuf& source, TNetBuf2& dest, int skip = 0, bool reset = false);

}
