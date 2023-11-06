#pragma once

#include <estd/string_view.h>

namespace embr { namespace coap {

namespace internal { inline namespace v1 {

// DEBT: Vague name and doesn't conform to pascal convention
using query = estd::pair<estd::string_view, estd::string_view>;

// DEBT: This can work, but we'll need the internal from_chars which
// takes iterators.  That likely comes with some kind of performance
// penalty.  Related DEBT from string_view and friends itself which
// duplicate data(), etc. 
//template <class Impl, class Int>
//static estd::from_chars_result from_string(
//    const estd::detail::basic_string<Impl>& s, Int& v)
// DEBT: Consider placing this directly into estd, seems useful, though a more thorough
// implementation of stoi and friends might be better
// DEBT: Filter Int by numeric/int
template <class Int>
static estd::from_chars_result from_string(
    const estd::string_view& s, Int& v)
{
    return estd::from_chars(s.begin(), s.end(), v);
}

template <class Int>
static estd::from_chars_result from_query(const query& q, const char* key, Int& v)
{
    static constexpr const char* TAG = "from_query";
    
    const estd::string_view value = estd::get<1>(q);

    if(estd::get<0>(q).compare(key) == 0)
    {
        // DEBT: from_string treats invalid trailing characters as
        // termination, but in our case we probaably want to indicate an error
        const estd::from_chars_result r = from_string(value, v);

        if(r.ec != 0)
        {
            ESP_LOGI(TAG, "from_query: key=%s cannot parse value=%.*s",
                key, value.size(), value.data());
        }
        else
        {
            long long debug_v = v;
            ESP_LOGD(TAG, "from_query: key=%s value=%lld", key, debug_v);
        }

        return r;
    }

    // DEBT: Use argument_out_of_domain instead, since
    // result_out_of_range is a from_string-generated error
    return { nullptr, estd::errc::result_out_of_range };
    //return { nullptr, estd::errc::argument_out_of_domain };
}



}}

}}