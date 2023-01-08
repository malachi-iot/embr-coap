#pragma once

#include "../../internal/factory.h"

#include <embr/platform/lwip/streambuf.h>
#include <embr/platform/lwip/transport.h>

// DEBT: All this really wants to live in embr project

namespace embr { namespace coap {

namespace experimental {

template <>
struct StreambufProvider<struct pbuf*>
{
    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;
};


template <>
struct StreambufProvider<embr::lwip::Pbuf>
{
    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;
};


// DEBT: LwIP has a natural pbuf size, seems to be PBUF_POOL_BUFSIZE.
// Consider making that the default value here, once we verify if that
// (or similar) is the minimum size a system/transport allocated pbuf
// will be anyway
template <unsigned N>
struct LwipPbufFactory
{
    static inline embr::lwip::Pbuf create()
    {
        return embr::lwip::Pbuf(N);
    }
};

}


}}