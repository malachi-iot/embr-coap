#pragma once

#include <estd/forward_list.h>
#include <estd/queue.h>
#include <estd/vector.h>

#include "coap-token.h"

namespace moducom { namespace coap { namespace experimental {

// passive push pull code to bridge transport level to application level
// kind of a 2nd crack at 'experimental-packet-manager'
template <class TNetBuf, template <class> class TAllocator = ::std::allocator>
class DataPump
{
public:
    typedef uint8_t addr_t[4];

private:
    struct Item
    {
        TNetBuf* netbuf;

        Item() {}

        Item(TNetBuf* netbuf) : netbuf(netbuf) {}
    };



    struct AddrMapper
    {
        addr_t address;
        coap::Header header; // for tkl and mid
        coap::layer1::Token token;
    };

    // does not want to initialize because Item has no default constructor
    // specifically underlying array is an array of Item...
    // TODO: Make underlying array capable of non-initialized data somehow, since
    // a queue is technically that to start with
    estd::layer1::queue<Item, 10> incoming;
    estd::layer1::queue<Item, 10> outgoing;

    //estd::forward_list<> retry;

    // might be better served by 2 different maps or some kind of memory_pool,
    // but we'll roll with just a low-tech vector for now
    estd::layer1::vector<AddrMapper, 10> addr_mapping;

    static bool find_mapper_by_addr(const AddrMapper& mapper)
    {

    }

public:
    // process data coming in from transport into coap queue
    void transport_in(TNetBuf& in, addr_t& addr);

    // provide a netbuf containing data to be sent out over transport, or NULLPTR
    // if no data is ready
    TNetBuf* transport_out(addr_t* addr_out)
    {
        if(outgoing.empty()) return NULLPTR;

        // can't quite do it because it wants AddrMapper (by way of iterator) to have ==
        // but I am not convinced that's the best approach
        //std::find(addr_mapping.begin(), addr_mapping.end(), find_mapper_by_addr);

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
