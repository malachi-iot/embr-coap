#pragma once

namespace moducom { namespace coap {

namespace experimental {

namespace event {

template <class TNetBuf, class TAddr>
struct Base
{
    TNetBuf& netbuf;
    const TAddr& addr;

    Base(TNetBuf& netbuf, const TAddr& addr) :
        netbuf(netbuf),
        addr(addr)
    {}
};

template <class TItem>
struct ItemBase
{
    // item residing in a tx or rx queue
    TItem& item;

    ItemBase(TItem& item) : item(item) {}
};

// appeared on transport
template <class TNetBuf, class TAddr>
struct TransportReceived : Base<TNetBuf, TAddr>
{
    TransportReceived(TNetBuf& netbuf, const TAddr& addr) :
        Base<TNetBuf, TAddr>(netbuf, addr)
    {}
};


// officially sent off over transport
template <class TNetBuf, class TAddr>
struct TransportSent : Base<TNetBuf, TAddr>
{
    TransportSent(TNetBuf& netbuf, const TAddr& addr) :
        Base<TNetBuf, TAddr>(netbuf, addr)
    {}
};

// queued for send out over transport
template <class TItem>
struct SendQueued : ItemBase<TItem>
{
    SendQueued(TItem& item) : ItemBase<TItem>(item) {}
};

// about to send over transport
template <class TItem>
struct SendDequeuing : ItemBase<TItem>
{
    SendDequeuing(TItem& item) : ItemBase<TItem>(item) {}
};

// sent over transport, now removing from send queue
// NOTE: retry logic keeps TNetBufs around, so a deallocation of netbuf
// may or may not follow
template <class TItem>
struct SendDequeued : ItemBase<TItem>
{
    SendDequeued(TItem& item) : ItemBase<TItem>(item) {}
};


// received from transport and now queued
template <class TItem>
struct ReceiveQueued : ItemBase<TItem>
{
    ReceiveQueued(TItem& item) : ItemBase<TItem>(item) {}
};


// pulling out of receive-from-transport queue
template <class TItem>
struct ReceiveDequeuing : ItemBase<TItem>
{
    ReceiveDequeuing(TItem& item) : ItemBase<TItem>(item) {}
};


// completed processing of one item from receive-from-transport queue
// right after this, TNetBuf will be deallocated
template <class TItem>
struct ReceiveDequeued : ItemBase<TItem>
{
    ReceiveDequeued(TItem& item) : ItemBase<TItem>(item) {}
};


}

}

}}
