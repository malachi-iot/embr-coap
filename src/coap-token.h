//
// Created by malachi on 12/29/17.
//

#ifndef MC_COAP_TEST_COAP_TOKEN_H
#define MC_COAP_TEST_COAP_TOKEN_H

#include <stdint.h>
#include <string.h>
#include "coap/platform.h"
#include "mc/memory-chunk.h"

#include <estd/array.h>
#include <estd/vector.h>
#include <estd/exp/buffer.h>

#ifdef FEATURE_CPP_INITIALIZER_LIST
#include <initializer_list>
#include <algorithm> // for std::copy
#endif

namespace moducom { namespace coap {

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
    typedef estd::layer1::vector<uint8_t, 8> base_t;

public:
    Token() {}

    Token(const layer1::Token& t, size_t tkl)
    {
        base_t::append(t.data(), tkl);
    }

    Token(const uint8_t* data, size_t tkl)
    {
        base_t::append(data, tkl);
    }

    template <class TImpl>
    Token& operator =(const estd::internal::allocated_array<TImpl>& assign_from)
    {
        base_t::assign(assign_from.clock(), assign_from.size());
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
    typedef estd::layer2::vector<const uint8_t, 8> base_t;

public:
    Token(const uint8_t* data, size_t tkl) :
            base_t(data, tkl)
    {
    }

    Token(const layer2::Token& token) : base_t(token.clock())
    {
        base_t::impl().size(token.size());
    }

    operator estd::const_buffer()
    {
        return estd::const_buffer(lock(), size());
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


#endif //MC_COAP_TEST_COAP_TOKEN_H
