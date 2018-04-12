#include "experimental-observer.h"
//#include "estd/queue.h"
#include "exp/netbuf.h"

namespace moducom { namespace coap { namespace experimental {


// implicitly layer3
class OutgoingPacketManager
{
    typedef struct { uint8_t raw[4]; } addr_t;

    class Item
    {
        addr_t addr;
        // eventually will probably be layer5 and/or templatized
        io::experimental::layer3::NetBufMemoryReader reader;
        io::experimental::layer3::NetBufMemoryReader writer;
    };
};


}}}
