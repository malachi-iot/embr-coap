#include "../../coap/platform.h"

#ifdef FEATURE_MCCOAP_ARDUINO

#include "udp-datapump.h"

namespace embr { namespace coap { namespace arduino {

void nonblocking_datapump_loop(UDP& udp, udp_datapump_t& datapump)
{
    if(!datapump.transport_empty())
    {
        udp_datapump_t::Item& item = datapump.transport_front();

        udp.beginPacket(item.addr().address, item.addr().port);
        //udp.write(); // write netbuf contents
        udp.endPacket();
    }

    int packetSize = udp.parsePacket();
    if(packetSize > 0)
    {

    }
}

}}}

#endif
