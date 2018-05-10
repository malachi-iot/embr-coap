#include "datapump.h"

namespace moducom { namespace coap { namespace experimental {

template <class TNetBuf, class TAddr, template <class> class TAllocator>
void DataPump<TNetBuf, TAddr, TAllocator>::transport_in(TNetBuf& in, const addr_t& addr)
{
#ifdef FEATURE_MCCOAP_DATAPUMP_INLINE
    incoming.emplace(in, addr);
#else
    Item item(in, addr);

    incoming.push(item);
#endif
}

}}}
