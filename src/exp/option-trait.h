#pragma once

#include <estd/internal/platform.h>
#include "../coap.h"

namespace embr { namespace coap { namespace experimental {

template <Option::Numbers number>
struct OptionTrait
{

};

struct OptionRuntimeTrait
{
    Option::ValueFormats format;
    const char* description;
    int min;
    int max;
};



template <Option::Numbers ...option_numbers>
struct OptionTraitContainer;

template <>
struct OptionTraitContainer<>
{
    static Option::ValueFormats format(Option::Numbers)
    {
        return Option::Unknown;
    }

    static int min(Option::Numbers) { return -1; }
    static int max(Option::Numbers) { return -1; }

    static OptionRuntimeTrait runtime_trait(Option::Numbers)
    {
        return OptionRuntimeTrait
        {
            Option::Unknown,
            NULLPTR,
            -1, -1
        };
    }
};

template <Option::Numbers option_number, Option::Numbers ...option_numbers>
struct OptionTraitContainer<option_number, option_numbers...>
{
    typedef OptionTrait<option_number> trait;
    typedef OptionTraitContainer<option_numbers...> base_t;

    static int min(Option::Numbers number)
    {
        return (number == option_number) ? trait::min() : base_t::min(number);
    }

    static int max(Option::Numbers number)
    {
        return (number == option_number) ? trait::max() : base_t::max(number);
    }

    static Option::ValueFormats format(Option::Numbers number)
    {
        if(number == option_number) return trait::format();

        return base_t::format(number);
    }

    static OptionRuntimeTrait runtime_trait(Option::Numbers number)
    {
        if(number == option_number)
        {
            return OptionRuntimeTrait
            {
                trait::format(),
                "N/A",
                trait::min(),
                trait::max()
            };
        }

        return base_t::runtime_trait(number);
    }
};

template <uint8_t min_value, uint16_t max_value>
struct OptionTraitBase
{
    static CONSTEXPR uint8_t min() { return min_value; }
    static CONSTEXPR uint16_t max() { return max_value; }
};

template <uint8_t min_value, uint16_t max_value>
struct OptionTraitString : public OptionTraitBase<min_value, max_value>
{
    static CONSTEXPR Option::ValueFormats format() { return Option::String; }
};

template <uint8_t min_value, uint16_t max_value>
struct OptionTraitOpaque : public OptionTraitBase<min_value, max_value>
{
    static CONSTEXPR Option::ValueFormats format() { return Option::Opaque; }
};

template <uint16_t max_value>
struct OptionTraitUInt : OptionTraitBase<0, max_value>
{
    static CONSTEXPR Option::ValueFormats format() { return Option::UInt; }
};

// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::IfMatch> : public OptionTraitOpaque<0, 8> {};


// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::UriPath> : public OptionTraitString<0, 255> {};

// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::LocationQuery> : public OptionTraitString<0, 255> {};

template <>
struct OptionTrait<Option::Block1> : public OptionTraitUInt<3> {};

template <>
struct OptionTrait<Option::Block2> : public OptionTraitUInt<3> {};

template <>
struct OptionTrait<Option::Size1> : public OptionTraitUInt<4> {};

template <>
struct OptionTrait<Option::Accept> : public OptionTraitUInt<2> {};

template <>
struct OptionTrait<Option::ProxyUri> : public OptionTraitString<1, 1034> {};

template <>
struct OptionTrait<Option::ProxyScheme> : public OptionTraitString<1, 255> {};

template <>
struct OptionTrait<Option::ContentFormat> : public OptionTraitUInt<2>
{
    static Option::ContentFormats to_native(uint16_t content_format)
    { return (Option::ContentFormats) content_format; }
};


template <>
struct OptionTrait<Option::ETag> : public OptionTraitOpaque<1, 8> {};

// couple compile time option trait specialiations to runtime
// query based on option number
typedef OptionTraitContainer<
            Option::IfMatch,
            Option::Size1,
            Option::UriPath,
            Option::LocationQuery,
            Option::ContentFormat,
            Option::Accept,
            Option::ETag,
            Option::ProxyUri,
            Option::ProxyScheme,
            Option::Block1,
            Option::Block2,
            Option::Size1
            >
option_traits_type;

}}}
