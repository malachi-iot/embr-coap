#include "datapump.h"

namespace moducom { namespace coap { namespace experimental {

template <class TNetBuf, template <class> class TAllocator>
void DataPump<TNetBuf, TAllocator>::transport_in(TNetBuf& in)
{
    Item item(&in);

    incoming.push(item);
}

}}}
