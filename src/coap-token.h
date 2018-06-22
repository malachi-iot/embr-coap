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
        // FIX: temporary measure to interact with estd::array's fluctuating code
        // base
        const uint8_t* data = &t[0];
        base_t::_append(data, tkl);
    }

    Token(const uint8_t* data, size_t tkl)
    {
        base_t::_append(data, tkl);
    }

    // TODO: put this into MemoryChunk itself
    // we don't expose ReadOnly constructor which can do this copy because
    // it might lose the const qualifiers
    operator moducom::pipeline::MemoryChunk::readonly_t() const
    {
        const uint8_t* data = clock(); // TODO: phase out memory chunks completely then we won't need a lingering lock
        return moducom::pipeline::MemoryChunk::readonly_t(data, size());
    }
    /*

    // TODO: Move this into underlying MemoryChunk
    bool operator ==(const Token& compare_to) const
    {
        if(compare_to.length() != length()) return false;

        return memcmp(data(), compare_to.data(), length()) == 0;
    } */
};


}


namespace layer3 {

class Token : public estd::layer2::vector<const uint8_t, 8>
{
    typedef estd::layer2::vector<const uint8_t, 8> base_t;

public:
    Token(const uint8_t* data, size_t tkl) : base_t(data)
    {
        base_t::impl().size(tkl);
    }

    Token(const layer2::Token& token) : base_t(token.clock())
    {
        base_t::impl().size(token.size());
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
