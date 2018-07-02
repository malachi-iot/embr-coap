#pragma once

#include <estd/span.h>
#include "../coap.h"

namespace moducom { namespace coap {

namespace experimental {

// 6/24/2018 revamped event system based on upcoming estd::experimental::subject code

// using only for typedef convenience
struct event_base
{
    typedef estd::const_buffer buffer_t;
    typedef internal::option_number_t option_number_t;
};

struct chunk_event_base : event_base
{
    buffer_t chunk;
    bool last_chunk;

    chunk_event_base() :
            chunk(NULLPTR, 0),
            last_chunk(true) {}

    chunk_event_base(const buffer_t& chunk, bool last_chunk) :
            chunk(chunk),
            last_chunk(last_chunk) {}
};

struct option_event : chunk_event_base
{
    option_number_t option_number;

    option_event(uint16_t n) :
            option_number((option_number_t)n) {}

    option_event(uint16_t n,
                 const buffer_t& chunk,
                bool last_chunk) :
            chunk_event_base(chunk, last_chunk),
            option_number((option_number_t)n)
    {}
};

// doing struct instead of typedef to ensure it overloads as
// a different type during on_notify
struct payload_event : chunk_event_base
{
    payload_event(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};


struct completed_event {};


struct header_event
{
    Header header;

    header_event(const Header& header) :
        header(header) {}
};


struct token_event : chunk_event_base
{
    token_event(
            const buffer_t& chunk,
            bool last_chunk) :
            chunk_event_base(chunk, last_chunk)
    {}
};


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


template <class TTransportDescriptor>
struct Transport
{
    typedef typename TTransportDescriptor::netbuf_t netbuf_t;
    typedef typename TTransportDescriptor::addr_t addr_t;

    typedef Base<netbuf_t, addr_t> base;

    typedef TransportReceived<netbuf_t, addr_t>
        transport_received;
    typedef TransportSent<netbuf_t, addr_t>
        transport_sent;

    struct transport_sending : base
    {
        transport_sending(netbuf_t& nb, const addr_t& addr) : base(nb, addr) {}
    };
};

template <class TItem>
struct Datapump
{
    typedef TItem item_t;

    typedef ItemBase<item_t> item_base;

    typedef ReceiveDequeuing<item_t>
        receive_dequeuing;
    typedef ReceiveDequeued<item_t>
        receive_dequeued;
    typedef ReceiveQueued<item_t>
        receive_queued;
    typedef SendQueued<item_t>
        send_queued;
    typedef SendDequeued<item_t>
        send_dequeued;
};

}

}

}}
