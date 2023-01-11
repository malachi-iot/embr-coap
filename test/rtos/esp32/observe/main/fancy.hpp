#pragma once

#include <embr/internal/scoped_guard.h>
#include <embr/platform/esp-idf/nvs.h>

// Fancy junk not specifically belonging in a reference app -- very likely we'll
// move it to framework level



namespace embr { namespace internal {

template <>
struct scoped_guard_traits<esp_idf::nvs::Handle>
{
    typedef esp_err_t status_type;
};


template <scoped_guard_fail_action action>
class scoped_guard<esp_idf::nvs::Handle, action> :
    public scoped_guard_base<esp_idf::nvs::Handle, false, action>
{
    typedef scoped_guard_base<esp_idf::nvs::Handle, false, action> base_type;

public:
    scoped_guard(const char* namespace_name, nvs_open_mode_t open_mode)
    {
        base_type::status(base_type::value().open(namespace_name, open_mode));
    }

    ~scoped_guard()
    {
        base_type::value().close();
    }

    // DEBT: Put this accessor out into scoped_guard_base
    esp_idf::nvs::Handle& operator*() { return base_type::value(); }
};


}

namespace esp_idf { namespace nvs {


template <class T>
esp_err_t get(const char* ns, const char* key, T* blob)
{
    embr::internal::scoped_guard<Handle> h(ns, NVS_READONLY);

    return get(*h, key, blob);
}


template <class T>
esp_err_t set(const char* ns, const char* key, T* blob)
{
    embr::internal::scoped_guard<Handle> h(ns, NVS_READWRITE);

    return set(*h, key, blob);
}



}}

}
