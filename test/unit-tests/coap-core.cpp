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
    static constexpr const char* rel() { return nullptr; }
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
struct coap_core_traits<v1> : coap_core_traits_base
{
    static constexpr const char* rel() { return "section"; }
};

template <>
struct coap_core_traits<v1_test> : coap_core_traits_base
{
    static constexpr const char* n() { return "Does Test Things"; }
    static constexpr embr::coap::Option::ContentFormats ct()
    {
        return embr::coap::Option::ApplicationXml;
    }
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
void default_core2(Streambuf& sb, estd::in_place_index_t<0>, path paths[], path* parent)
{

}

template <class Streambuf>
void print_path(Streambuf& sb, path paths[], int idx, char delim = '/')
{
    if(idx == -1) return;

    print_path(sb, paths, paths[idx].third, delim);

    sb.sputn(paths[idx].first.data(), paths[idx].first.size());
    sb.sputc(delim);
}

template <size_t id, class Streambuf>
void default_core2(Streambuf& sb, estd::in_place_index_t<id>, path paths[], path* parent)
{
    using traits = coap_core_traits<id>;
    path* current = &paths[id];

    sb.sputc('<');
    sb.sputc('/');
    print_path(sb, paths, current->second);
    sb.sputc('>');

    if (traits::ct() != embr::coap::Option::TextPlain)
    {
        char buf[4];

        sb.sputn(";ct=", 4);

        // Due to https://github.com/malachi-iot/estdlib/issues/28 we specify 3 here instead
        // of 4
        //estd::to_chars_result r = estd::to_chars_opt(buf, buf + 3, (uint16_t)traits::ct());
        estd::to_chars_result r = estd::detail::to_chars<10>(buf, buf + 3, (uint16_t)traits::ct());
        sb.sputn(r.ptr, buf + 4 - r.ptr);
    }

    if (traits::n() != nullptr)
    {
        sb.sputn(";n=\"", 4);
        sb.sputn(traits::n(), strlen(traits::n()));
        sb.sputc('"');
    }

    if (traits::rel() != nullptr)
    {
        sb.sputn(";rel=\"", 6);
        sb.sputn(traits::rel(), strlen(traits::rel()));
        sb.sputc('"');
    }

    sb.sputc('\n');

    if (id == MCCOAP_URIPATH_NONE) return;

    if (traits::rel() == "section") return;

    default_core2(sb, estd::in_place_index_t<id - 1>{}, paths, current);
}

// See also experimental2-tests for CoRE experimental code

TEST_CASE("CoAP CoRE tests", "[coap-core]")
{
    estd::layer1::ostringstream<128> out;

    default_core2(*out.rdbuf(), estd::in_place_index_t<v1_test>{}, paths, nullptr);

    //INFO(out.rdbuf()->str().data());
    puts(out.rdbuf()->str().data());
}