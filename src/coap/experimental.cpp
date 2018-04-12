#include "experimental-observer.h"
//#include "estd/queue.h"
//#include "exp/netbuf.h"

namespace moducom { namespace coap { namespace experimental {


// implicitly layer2+layer3
class OutgoingPacketManager
{
    typedef struct { uint8_t raw[4]; } addr_t;

    class Item
    {
        addr_t addr;
        //io::experimental::layer3::NetBufMemoryReader reader;
        //io::experimental::layer3::NetBufMemoryReader<1> writer;
    };
};


}}}
