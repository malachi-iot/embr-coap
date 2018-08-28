#pragma once

#include "misc.h"

namespace embr { namespace experimental {


template <class TNetBuf, class TNetBuf2>
void coap_netbuf_copy(TNetBuf& source, TNetBuf2& dest, int skip, bool reset)
{
    if(reset)
    {
        source.first();
        dest.first();
    }

    const uint8_t* src = source.processed();
    uint8_t* dst = dest.unprocessed();
    size_t src_len = source.length_processed();
    size_t dst_len = dest.length_unprocessed();

    if(skip > src_len)
    {
        // error
    }

    src += skip;
    src_len -= skip;

    for(;;)
    {
        for (; src_len > 0 && dst_len > 0; src++, dst++, src_len--, dst_len--)
        {
            *dst = *src;
        }

        dest.advance(dest.length_unprocessed() - dst_len);

        // if src wasn't completely copied
        if(src_len > 0)
        {
            // TODO: need to make and call TBD next(bufsize) allocator
            // dst ran out, so see if we can advance forward for dst
            if(dest.end())
            {
                // error
            }

            dest.next(); // TODO: this will likely want to change when tri-state return is implemented for next()

            dst = dest.unprocessed();
            dst_len = dest.length_unprocessed();
        }
        // if src wasn't completely copied based on presence of additional chunk
        // src_len == 0 here, or in other words, src ran out before (or during) dst
        else if(!source.end())
        {
            // it's possible we ended both src and dst on a chunk boundary, let's check
            // for that
            if(dst_len == 0)
            {
                // FIX: dedup code
                // TODO: need to make and call TBD next(bufsize) allocator
                // dst ran out, so see if we can advance forward for dst
                if(dest.end())
                {
                    // error
                }

                dest.next(); // TODO: this will likely want to change when tri-state return is implemented for next()

                dst = dest.unprocessed();
                dst_len = dest.length_unprocessed();
            }

            source.next();
            src = source.processed();
            src_len = source.length_processed();
        }
        else
            // src was completely copied
            break;
    }
}


}}
