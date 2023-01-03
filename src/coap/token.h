#pragma once

#include <estd/cstdint.h>
#include "platform.h"
//#include <mc/memory-chunk.h>

#include <estd/array.h>
#include <estd/vector.h>
#include <estd/span.h>

#ifdef FEATURE_CPP_INITIALIZER_LIST
#include <initializer_list>
#include <algorithm> // for std::copy
#endif

// FIX: Token layering is confusing and should be revisited
// layer1 = inline 8 bytes, no explicit size, someone else tracks it
// layer2 = inline 8 bytes max, explicit size
// layer3 = ptr 8 bytes max, explicit size
namespace embr { namespace coap {

namespace layer1 {

// https://tools.ietf.org/html/rfc7252#section-5.3.1
// semi-duplicate of one in coap_transmission
class Token : public estd::array<uint8_t, 8>
{
    typedef estd::array<uint8_t, 8> base_t;

public:
#ifdef FEATURE_CPP_INITIALIZER_LIST
    Token(std::initializer_list<uint8_t> l) : base_t(l)
    {
    }

    Token() {}
#endif
};

}

namespace layer2 {

// https://tools.ietf.org/html/rfc7252#section-5.3.1
// semi-duplicate of one in coap_transmission
class Token : public estd::layer1::vector<uint8_t, 8>
{
    typedef estd::layer1::vector<uint8_t, 8> base_type;

public:
    Token() {}

    Token(const layer1::Token& t, size_t tkl)
    {
        base_type::append(t.data(), tkl);
    }

    Token(const uint8_t* data, size_t tkl)
    {
        base_type::append(data, tkl);
    }

    Token(const estd::span<const uint8_t>& buffer)
    {
        base_type::append(buffer.data(), buffer.size());
    }

    template <class TImpl>
    Token& operator =(const estd::internal::allocated_array<TImpl>& assign_from)
    {
        base_type::assign(assign_from.clock(), assign_from.size());
        assign_from.cunlock();
        // can't do this because I think layer1::vector implicitly deleted it
        //base_t::operator =(assign_from);
        return *this;
    }
};


}


namespace layer3 {

// NOTE: This differs slightly from a regular const_buffer approach in that
// we expect the underlying data buffer to actually be 8 bytes large,
// even if tkl itself is smaller.  As of this writing that behavior is not used
// though
class Token : public estd::layer2::vector<const uint8_t, 8>
{
    typedef estd::layer2::vector<const uint8_t, 8> base_type;

public:
    typedef estd::span<const uint8_t> const_buffer;

    Token(const uint8_t* data, size_t tkl) :
        base_type(data, tkl)
    {
    }

    Token(const const_buffer& span) :
        base_type(span.data(), span.size())
    {
    }
    // DEBT: Remove this or make it more explicit, I don't like taking 
    // address of a possibly temporary variable in layer2::Token
    Token(const layer2::Token& token) : base_type(token.clock())
    {
        base_type::impl().size(token.size());
    }

    // TODO: Had to remove constexpr because c++20 tells us ultimately estd::internal::layer3::buffer
    // does not have a trivial default constructor or a constexpr constructor, which is true.
    // we can change that pretty easily when the time comes
    inline operator const_buffer() const
    {
        return const_buffer(data(), size());
    }
};

}

class SimpleTokenGenerator
{
    uint32_t current;

public:
    SimpleTokenGenerator() : current(0) {}

    void generate(coap::layer2::Token* token);

    // FIX: Temporarily making setter always available until architecture settles down
//#ifdef DEBUG
    void set(uint32_t current) { this->current = current; }
//#endif
};



}}
