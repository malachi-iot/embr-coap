#pragma once

namespace moducom { namespace coap { namespace experimental {


template <Option::Numbers number>
struct OptionTrait
{

};


template <uint8_t min_value, uint16_t max_value>
struct OptionTraitBase
{
    static uint8_t min() { return min_value; }
    static uint16_t max() { return max_value; }
};

template <uint8_t min_value, uint16_t max_value>
struct OptionTraitString : public OptionTraitBase<min_value, max_value>
{
    static Option::ValueFormats format() { return Option::String; }
};

template <uint8_t min_value, uint16_t max_value>
struct OptionTraitOpaque : public OptionTraitBase<min_value, max_value>
{
    static Option::ValueFormats format() { return Option::Opaque; }
};

struct OptionTraitUInt
{
    static uint8_t min() { return 0; }
    static Option::ValueFormats format() { return Option::UInt; }
};

// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::IfMatch> : public OptionTraitOpaque<0, 8> {};


// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::Size1> : public OptionTraitOpaque<0, 4> {};


// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::UriPath> : public OptionTraitString<0, 255> {};

// experimental, don't know if compiletime-only versions of this area really *that* helpful
template <>
struct OptionTrait<Option::LocationQuery> : public OptionTraitString<0, 255> {};


template <>
struct OptionTrait<Option::ContentFormat> : public OptionTraitUInt
{
    static uint8_t max() { return 2; }
    static Option::ContentFormats to_native(uint16_t content_format)
    { return (Option::ContentFormats) content_format; }
};


template <>
struct OptionTrait<Option::ETag> : public OptionTraitOpaque<1, 8> {};


}}}
