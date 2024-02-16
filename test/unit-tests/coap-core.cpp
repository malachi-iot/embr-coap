#include <catch2/catch.hpp>

#include <coap/header.h>
#include <coap/decoder/uri.h>
#include <coap/context.h>
#include <coap/option.h>
#include <coap/platform/api.h>  // DEBT: Needed for CoRE... but don't want it to be

#include <estd/sstream.h>

using path = embr::coap::internal::UriPathMap;

enum paths_type
{
    v1,
    v1_test
};

// All EXPERIMENTAL at this point

path paths[] =
{
    { "v1",     v1,         MCCOAP_URIPATH_NONE },
    { "test",   v1_test,    v1 },
    EMBR_COAP_CoRE_PATHS()
};

struct coap_core_traits_base
{
    static constexpr const char* n() { return nullptr; }
    static constexpr embr::coap::Option::ContentFormats ct()
    {
        return embr::coap::Option::TextPlain;
    }
};

template <int, class = void>
struct coap_core_traits : coap_core_traits_base
{
};

template <>
struct coap_core_traits<v1_test> : coap_core_traits_base
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

template <class Streambuf>
void default_core2(Streambuf& sb, estd::in_place_index_t<0>)
{

}

template <size_t id, class Streambuf>
void default_core2(Streambuf& sb, estd::in_place_index_t<id>)
{
    if (id == MCCOAP_URIPATH_NONE) return;

    default_core2(sb, estd::in_place_index_t<id - 1>{});
}

TEST_CASE("CoAP CoRE tests", "[coap-core]")
{
    estd::layer1::ostringstream<128> out;

    default_core2<100>(*out.rdbuf(), estd::in_place_index_t<100>{});
}