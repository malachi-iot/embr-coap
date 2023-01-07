#pragma once

#include <estd/algorithm.h>
#include <estd/span.h>

namespace embr { namespace coap {

namespace internal {

// DEBT: Eventually we want this to be estd::byte, not uint8_t
typedef estd::span<const uint8_t> const_buffer;

// DEBT: I don't fully understand
// https://stackoverflow.com/questions/60633668/why-does-stdspan-lack-the-comparison-operators
// But I do think using string_view would be generally superior for token slinging
// DEBT: On its own merit, this might be slightly better served by creating
// estd::equal(begin1, end1, begin2, end2) as is in regular std - though this one here is slightly more efficient
template <class TChar, estd::size_t Extent1, estd::size_t Extent2>
inline bool equal(const estd::span<TChar, Extent1>& lhs,
    const estd::span<TChar, Extent2>& rhs)
{
    if(rhs.size() != lhs.size()) return false;

    return estd::equal(lhs.begin(), lhs.end(), rhs.begin());
}

}

}}