#pragma once

#include <estd/forward_list.h>
#include <estd/vector.h>
#include <mc/memory-pool.h>
#include "coap/decoder/netbuf.h"
#include <stdint.h> // for uint8_t

namespace moducom { namespace coap { namespace experimental {


// only CON messages live here, expected to be shuffled here right out of datapump
// they are removed from our retry_list when an ACK is received, or when our backoff
// logic finally expires
// in support of https://tools.ietf.org/html/rfc7252#section-4.2
template <class TNetBuf>
class Retry
{
public:
    typedef uint8_t addr_t[4];
    typedef ::time_t time_t;

    struct Metadata
    {
        // number of retries attempted so far
        uint8_t retry_count;

        // when to send it by
        time_t due;

        Metadata() :
            retry_count(0) {}
    };

    // proper MID and Token are buried in netbuf, so don't need to be carried
    // seperately
    // TODO: Utilize ObservableSession as a base once we resolve netbuf-inline behaviors here
    struct Item : Metadata
    {
        // where to send retry
        addr_t addr;

        // what to send
        // right now hard-wired to non-line netbuf style
        TNetBuf* m_netbuf;

        TNetBuf& netbuf() { return m_netbuf; }

        // get MID from sent netbuf, for incoming ACK comparison
        uint16_t mid() const
        {
            // TODO: optimize and use header decoder only and directly
            NetBufDecoder<TNetBuf&> decoder(netbuf());

            return decoder.header().message_id();
        }

        // get Token from sent netbuf, for incoming ACK comparison
        // TODO: Make a layer3::Token which can lean on netbuf contents,
        // as right now netbuf has a requirement which it must at least support
        // the first 12 bytes (head + token) without fragementation
        coap::layer2::Token token() const
        {
            // TODO: optimize and use header & token decoder only and directly
            NetBufDecoder<TNetBuf&> decoder(netbuf());
            coap::layer2::Token token;

            decoder.header();
            decoder.process_token_experimental(&token);

            return token;
        }

        // needed for unallocated portions of vector
        Item() {}

        Item(const addr_t& a, TNetBuf* netbuf) :
                m_netbuf(netbuf)
        {
            // TODO: assign addr
        }
    };

private:
    //Using raw linked list pool for now until we get a better sorted
    // solution in place
    typedef moducom::mem::LinkedListPool<int, 10> llpool_t;
    //typedef moducom::mem::LinkedListPoolNodeTraits<int, 10> node_traits_t;

    /*
    typedef estd::forward_list<int,
            estd::inlinevalue_node_traits<estd::experimental::forward_node_base,
                    mem::LinkedListPool */

    typedef estd::layer1::vector<Item, 10> list_t;

    // TODO: this should eventually be a priority_queue or similar
    list_t retry_list;

public:
    Retry() {}

    bool enqueue(const addr_t& addr, TNetBuf& netbuf)
    {
        // TODO: ensure it's sorted by 'due'
        // for now just brute force things
        Item item(addr, &netbuf);

        // TODO: revamp the push_back code to return success or fail
        retry_list.push_back(item);

        return true;
    }

    // call this to get next item for transport to send, or NULLPTR if nothing
    // keep in mind Item shall have 'due' in there to indicate when item should
    // *actually* be sent, it is up to consumer to heed this
    //const Item* front() const
    Item* front()
    {
        // TODO: make this const eventually ... though maybe
        // we can't since transport ultimately will want to diddle with netbuf
        Item* best = NULLPTR;

        //for(Item* v : retry_list)
        typename list_t::iterator i = retry_list.begin();

        while(i != retry_list.end())
        {
            Item& v = *i;
            if(best == NULLPTR)
                best = &v;
            else if(v.due < best->due)
                best = &v;

            i++;
        }

        return best;
    }

    // called when ACK is received to determine if we should remove anything from the
    // retry_queue
    void ack_received(const addr_t& from_addr, uint16_t mid, const coap::layer2::Token& token)
    {

    }

    // call this after front() is called and its contained 'due' has passed
    // will:
    //   a) evaluate first (front()) item to see if current_time > due and
    //      1. if so, requeue for later retry attempt again
    //      2. if not, do nothing
    //   b)
    void service(time_t current_time)
    {

    }
};


}}}
