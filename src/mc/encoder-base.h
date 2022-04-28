#pragma once

#include "mc/netbuf.h"

namespace embr {

template <class TSize = size_t>
class EncoderBase
{
protected:
    typedef TSize size_type;

    // represents how many bytes were written during last public/high level encode operation
    // only available if encode operation comes back as false.  Undefined when operation
    // succeeds
    size_type m_written;

    void written(size_type w) { m_written = w; }

};



}
