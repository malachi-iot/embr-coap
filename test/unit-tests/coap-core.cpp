#include <catch2/catch.hpp>

#include <coap/header.h>
#include <coap/decoder/uri.h>
#include <coap/context.h>
#include <coap/option.h>
#include <coap/platform/api.h>  // DEBT: Needed for CoRE... but don't want it to be

using path = embr::coap::internal::UriPathMap;

enum paths_type
{
    v1,
    v1_test
};

path paths[] =
{
    { "v1",     v1,         MCCOAP_URIPATH_NONE },
    { "test",   v1_test,    v1 },
    EMBR_COAP_CoRE_PATHS()
};

template <int, class = void>
struct coap_core_traits
{
    static constexpr const char* n() { return nullptr; }
    static constexpr embr::coap::Option::ContentFormats ct()
    {
        return embr::coap::Option::TextPlain;
    }
};

template <>
struct coap_core_traits<v1_test>
{
    static constexpr const char* n() { return "Does Test Things"; }
};

template <class Streambuf>
void default_core(Streambuf& sb, path paths[], int count)
{
    // out = payload
    estd::detail::basic_ostream<Streambuf> out;

    for(int i = 0; i < count; i++)
    {
        
    }
}

TEST_CASE("CoAP CoRE tests", "[coap-core]")
{

}