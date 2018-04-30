#pragma once

#include <estd/forward_list.h>
#include <estd/queue.h>

namespace moducom { namespace coap { namespace experimental {

// passive push pull code to bridge transport level to application level
// kind of a 2nd crack at 'experimental-packet-manager'
template <class TNetBuf, template <class> class TAllocator = ::std::allocator>
class DataPump
{
    struct Item
    {
        TNetBuf* netbuf;

        Item() {}

        Item(TNetBuf* netbuf) : netbuf(netbuf) {}
    };

    // does not want to initialize because Item has no default constructor
    // specifically underlying array is an array of Item...
    // TODO: Make underlying array capable of non-initialized data somehow, since
    // a queue is technically that to start with
    estd::queue<Item, estd::layer1::deque<Item, 10> > incoming;
    estd::queue<Item, estd::layer1::deque<Item, 10> > outgoing;

    //estd::forward_list<> retry;

public:
    // process data coming in from transport into coap queue
    void transport_in(TNetBuf& in);

    // provide a netbuf containing data to be sent out over transport, or NULLPTR
    // if no data is ready
    TNetBuf* transport_out()
    {
        if(outgoing.empty()) return NULLPTR;

        const Item& f = outgoing.front();

        outgoing.pop();

        return f.netbuf;
    }

    // enqueue complete netbuf for outgoing transport to pick up
    void enqueue_out(TNetBuf& out)
    {
        outgoing.push(Item(&out));
    }

    // dequeue complete netbuf which was queued from transport in
    TNetBuf* dequeue_in()
    {
        if(incoming.empty()) return NULLPTR;

        const Item& f = incoming.front();

        incoming.pop();

        return f.netbuf;
    }
};

}}}
