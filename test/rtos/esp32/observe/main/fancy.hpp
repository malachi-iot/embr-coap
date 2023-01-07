#pragma once

#include "nvs_flash.h"
#include "nvs.h"

#include <embr/internal/scoped_guard.h>

// Fancy junk not specifically belonging in a reference app -- very likely we'll
// move it to framework level

// NOTE: esp-idf has its own C++ thing going on with full unique_ptr and virtual
// methods.  We're going a different toute
namespace embr { namespace esp_idf { namespace nvs {

// NOTE: Since they have varieties of handles in their c++ implementation, 
// I expect we'll need some template tricks or namespace segregation.  Not doing that now though.
struct Handle
{
    nvs_handle_t h;

public:
    err_t open(const char* namespace_name, nvs_open_mode_t open_mode)
    {
        return nvs_open(namespace_name, open_mode, &h);
    }

    err_t get_blob(const char* key, void* buffer, std::size_t* sz)
    {
        return nvs_get_blob(h, key, buffer, sz);
    }

    void close()
    {
        nvs_close(h);
    }

    operator nvs_handle_t() const { return h; }
};

}}}


namespace embr { namespace internal {

template <>
struct scoped_guard_traits<esp_idf::nvs::Handle>
{
    typedef esp_err_t status_type;
};


// DEBT: Consolidate this with the one in power.cpp test
template <>
struct scoped_status_traits<esp_err_t>
{
    static bool good(esp_err_t e) { return e == ESP_OK; }
    inline static void log(esp_err_t e)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(e);
    }
    inline static void assert_(esp_err_t e)
    {
        ESP_ERROR_CHECK(e);
    }
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
};

}}
