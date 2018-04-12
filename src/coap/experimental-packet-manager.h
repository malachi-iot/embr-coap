#pragma once

//#include "experimental-observer.h"
//#include "estd/queue.h"
#include "exp/netbuf.h"
#include "mc/memory-pool.h"

namespace moducom { namespace coap { namespace experimental {

// has access to simplistic memory allocation/management (good candidate is to use
// up and coming memory alloc stuff from moducom_memory_lib)
// and can coordinate memory chunks from there
class MultiNeBufMemoryWriter
{

};

struct ExpAddr { uint8_t raw[4]; };

template <class TNetBuf, typename TAddress>
class OutgoingPacketManagerBase
{
    typedef TAddress addr_t;

    class Item
    {
        addr_t addr;
        TNetBuf* netbuf;

        bool active;
        bool m_ready_to_send;

    public:
        // magic which helps pool code work
        bool is_active() const { return active; }

        bool ready_to_send() const { return m_ready_to_send; }
        void ready_to_send(bool value) { m_ready_to_send = value; }

        Item() :
            active(false),
            m_ready_to_send(false)
        {}

        Item(TNetBuf* netbuf, bool cts = false) :
            netbuf(netbuf),
            active(true),
            m_ready_to_send(cts)
        {}

        //~Item() { active(false); }
    };

    dynamic::PoolBase<Item, 10> pool;

public:
    typedef Item* item_t;

    item_t add(TNetBuf* netbuf)
    {
        return pool.allocate(netbuf);
    }
};

struct OutgoingPacketManager : public OutgoingPacketManagerBase<io::experimental::layer5::INetBuf, ExpAddr>
{

};

}}}
