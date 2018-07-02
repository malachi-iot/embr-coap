#pragma once

namespace moducom { namespace coap {

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
    // NOTE: has diminished value when using INLINE/move semantic flavor
    virtual bool on_message_transmitted(TNetBuf* netbuf, const TAddr& addr) = 0;
};

}}
