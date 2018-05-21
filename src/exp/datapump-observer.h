#pragma once

namespace moducom { namespace coap {

// can and should be used also to do semi-partial coap netbuf copies, so that we can
// simulate a header rewrite.  As such, be mindful that default behavior is for this to NOT
// reset to 'first' but instead to copy as-is from current positions
// skip represents number of bytes from beginning of source to skip before
// initiating copy.  NOTE: this skip may obviate reset flag
template <class TNetBuf, class TNetBuf2>
void netbuf_copy(TNetBuf& source, TNetBuf2& dest, int skip = 0, bool reset = false);

// Chances are passing in netbuf/addr is redundant since observer and retry logic will
// already have this info, but could be useful down the line so keeping it
template <class TNetBuf, class TAddr>
struct IDataPumpObserver
{
    // LwIP *might* need this, I've heard reports sometimes that after sending a netbuf it goes invalid.  Need to
    // doublecheck because this could strongly affect retry techniques.
    // reusing netbufs is safe, according to:
    // http://lwip.100.n7.nabble.com/netconn-freeing-netbufs-after-netconn-recv-td4145.html
    // For now going to presume we can reuse netbufs for our retry code, but since observer code basically
    // requires a netbuf-copyer, prep that too
    virtual void on_message_transmitting(TNetBuf* netbuf, const TAddr& addr) = 0;

    // true = we have taken ownership of netbuf, signal to caller not to deallocate
    // false = we are not interested, caller maintains ownership of netbuf
    virtual bool on_message_transmitted(TNetBuf* netbuf, const TAddr& addr) = 0;
};

}}
