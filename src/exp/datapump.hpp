#include "datapump.h"

namespace moducom { namespace coap { namespace experimental {

template <class TNetBuf>
void DataPump<TNetBuf>::transport_in(TNetBuf& in)
{
    Item item(&in);

    incoming.push(item);
}

}}}
